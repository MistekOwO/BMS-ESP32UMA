
#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "class/hid/hid_device.h"
#include "driver/gpio.h"
#include "usbSendData.h"
#include "driver/gptimer.h"
#include "config.h"



#define MaxEncoder 640
#define MinEncoder -640
static int buttons[] = {W1Button,W2Button,W3Button,W4Button,B1Button,B2Button,B3Button,F1Button,F2Button,F3Button,F4Button};
static const char *TAG = "example";

_Atomic volatile int lastState;
_Atomic volatile int encoderValue;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
     int a = gpio_get_level(EncoderA);
     int b = gpio_get_level(EncoderB);
     int state = (a << 1) | b;

    switch (state)
    {
        case 0b01:
        {
            switch (lastState)
            {
                case 0b11: encoderValue++; break;
                case 0b00: encoderValue--; break;
                default:
                    break;
            }
        }break;
        case 0b00:
        {
            switch (lastState)
            {
                case 0b01: encoderValue++; break;
                case 0b10: encoderValue--; break;
                default:
                    break;
            }
        }break;
        case 0b10:
        {
            switch (lastState)
            {
                case 0b00: encoderValue++; break;
                case 0b11: encoderValue--; break;
                default:
                    break;
            }           
        }break;
        case 0b11:
        {
            switch (lastState)
            {
                case 0b10: encoderValue++; break;
                case 0b01: encoderValue--; break;
                default:
                    break;
            }            
        }break;

    default:
        break;
    }

    if(encoderValue == 640) encoderValue = -640;
    else if(encoderValue == -640) encoderValue = 640;
    
    lastState = state;     
}

 void configureButtons()
 {
    for(int i=0;i<sizeof(buttons)/sizeof(int);i++)
    {
        gpio_config_t ledGpioConfig = {.pin_bit_mask = (1ULL << buttons[i]),.mode = GPIO_MODE_INPUT,.pull_up_en = GPIO_PULLUP_DISABLE,.pull_down_en = GPIO_PULLDOWN_ENABLE,.intr_type = GPIO_INTR_DISABLE};
        gpio_config(&ledGpioConfig);
        gpio_input_enable(buttons[i]);
        gpio_set_pull_mode(buttons[i],GPIO_PULLDOWN_ONLY);
    }
    
 }

 void configureEncoder()
 {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << EncoderA) | (1ULL << EncoderB),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,  
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&io_conf);

    int a = gpio_get_level(EncoderA);
    int b = gpio_get_level(EncoderB);
    lastState = (a << 1) | b;
    gpio_install_isr_service(0);
    gpio_isr_handler_add(EncoderA, gpio_isr_handler, (void*) EncoderA);
    gpio_isr_handler_add(EncoderB, gpio_isr_handler, (void*) EncoderB);
 }


#define TUSB_DESC_TOTAL_LEN      (TUD_CONFIG_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)


const uint8_t hid_report_descriptor[] = {
    HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP     ),
    HID_USAGE      ( HID_USAGE_DESKTOP_GAMEPAD  ),
    HID_COLLECTION ( HID_COLLECTION_APPLICATION ),
        HID_USAGE ( HID_USAGE_DESKTOP_X ),
        HID_USAGE ( HID_USAGE_DESKTOP_Y ),
        HID_LOGICAL_MIN  (0x81),  
        HID_LOGICAL_MAX  (0x7f),  
        HID_REPORT_SIZE  (8),
        HID_REPORT_COUNT (2),
        HID_INPUT ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ),
        HID_USAGE_PAGE  ( HID_USAGE_PAGE_BUTTON ),
        HID_USAGE_MIN   (1),
        HID_USAGE_MAX   (16),
        HID_LOGICAL_MIN (0),
        HID_LOGICAL_MAX (1),
        HID_REPORT_SIZE (1),
        HID_REPORT_COUNT(16),
        HID_INPUT ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ),
    HID_COLLECTION_END,
};


const char* hid_string_descriptor[5] = {
    
    (char[]){0x09, 0x04},  
    "Mist",             
    "BMS CON",      
    "123456",              
    "BMS HID interface", 
};


static const uint8_t hid_configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(0, 4, false, sizeof(hid_report_descriptor), 0x81, 16, 1),
};


uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    return hid_report_descriptor;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;

    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
}

void app_main(void)
{
    
    ESP_LOGI(TAG, "USB initialization");
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,
        .string_descriptor = hid_string_descriptor,
        .string_descriptor_count = sizeof(hid_string_descriptor) / sizeof(hid_string_descriptor[0]),
        .external_phy = false,
#if (TUD_OPT_HIGH_SPEED)
        .fs_configuration_descriptor = hid_configuration_descriptor, 
        .hs_configuration_descriptor = hid_configuration_descriptor,
        .qualifier_descriptor = NULL,
#else
        .configuration_descriptor = hid_configuration_descriptor,
#endif // TUD_OPT_HIGH_SPEED
    };
    
gptimer_config_t debounce_config = {.clk_src = SOC_MOD_CLK_APB,.direction = GPTIMER_COUNT_UP,.intr_priority=0 ,.resolution_hz = 10000};
gptimer_handle_t debouce_handler;
gptimer_new_timer(&debounce_config,&debouce_handler);
gptimer_enable(debouce_handler);
gptimer_set_raw_count(debouce_handler,0);
    configureButtons();
    configureEncoder();
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    ESP_LOGI(TAG, "USB initialization DONE");
    uint8_t buttonsDetect[sizeof(buttons)/sizeof(int)];
    uint8_t buttonsAfterDebounce[10];
    memset(buttonsDetect,0,10);
    memset(buttonsAfterDebounce,0,10);
    int ArrayPos = 0;
    int numberOfChecks = 0;
    while (1) {   
            numberOfChecks = 0;
        if (tud_mounted()) {
            //debounce start
            while(numberOfChecks < 4)
            {
                for(int i=0;i<sizeof(buttons)/sizeof(int);i++)
                {
                        if(gpio_get_level(buttons[i]) == 1)
                        {
                            buttonsDetect[i]++;
                        }
                }
                numberOfChecks++;
            }
            ArrayPos = 0;
            for(int i=0;i<sizeof(buttons)/sizeof(int);i++)
            {
                if(buttonsDetect[i] >= 2)
                {
                    buttonsAfterDebounce[ArrayPos] = buttons[i];
                    ArrayPos += 1;
                }
                
            }
               
        sendControllerInputs(buttonsAfterDebounce,encoderValue);
        memset(buttonsDetect,0,sizeof(buttons)/sizeof(int));
        memset(buttonsAfterDebounce,0,10);
        
        }
    }
}