/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "class/hid/hid_device.h"
#include "driver/gpio.h"
#include "usbSendData.h"
#include "driver/gptimer.h"



#define W1Button 35
#define W2Button 34
#define W3Button 13
#define W4Button 42
#define B1Button 5
#define B2Button 16
#define B3Button 17
#define F1Button 3
#define F2Button 36
#define MaxEncoder 635;
#define MinEncoder -640;
static int buttons[] = {W1Button,W2Button,W3Button,W4Button,B1Button,B2Button,B3Button,F1Button,F2Button};
#define APP_BUTTON (GPIO_NUM_0) // Use BOOT signal by default
static const char *TAG = "example";
static QueueHandle_t gpio_evt_queue;
int lastState;
int encoderValue;


static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}



void readEncoder()
{
    int a = gpio_get_level(11);
    int b = gpio_get_level(12);
    int state = (a << 1) | b;

    if(state == 0b01)
    {
        if (lastState == 0b00) encoderValue++;
        else if(lastState == 0b11) encoderValue--;
    }
    else if(state == 0b00)
    {
        if(lastState == 0b01) encoderValue++;
        else if(lastState == 0b10) encoderValue--;
    }
    else if(state == 0b10)
    {
        if(lastState == 0b00) encoderValue++;
        else if(lastState == 0b11) encoderValue--;
    }
    else if(state == 0b11)
    {
        if(lastState == 0b10) encoderValue++;
        else if(lastState == 0b01) encoderValue--;
    }

    if(encoderValue == 635) encoderValue = -640;
    if(encoderValue == -640) encoderValue = 635;
    lastState = state;

}
static void gpio_task_example(void* arg)
{
    uint32_t io_num;
    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            readEncoder();
        }
    }
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
        .pin_bit_mask = (1ULL << 11) | (1ULL << 12),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,   // encoders usually need pull-up
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&io_conf);

    // initial state of encoder
    int a = gpio_get_level(11);
    int b = gpio_get_level(12);
    lastState = (a << 1) | b;
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(11, gpio_isr_handler, (void*) 11);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(12, gpio_isr_handler, (void*) 12);
 }
/************* TinyUSB descriptors ****************/

#define TUSB_DESC_TOTAL_LEN      (TUD_CONFIG_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)

/**
 * @brief HID report descriptor
 *
 * In this example we implement Keyboard + Mouse HID device,
 * so we must define both report descriptors
 */
const uint8_t hid_report_descriptor[] = {
    //TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD)),
    HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP     ),
    HID_USAGE      ( HID_USAGE_DESKTOP_GAMEPAD  ),
    HID_COLLECTION ( HID_COLLECTION_APPLICATION ),
        // X, Y
        HID_USAGE ( HID_USAGE_DESKTOP_X ),
        HID_USAGE ( HID_USAGE_DESKTOP_Y ),
        HID_LOGICAL_MIN  (0x81),  // -127
        HID_LOGICAL_MAX  (0x7f),  // +127
        HID_REPORT_SIZE  (8),
        HID_REPORT_COUNT (2),
        HID_INPUT ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ),

        // 8 przycisków
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

/**
 * @brief String descriptor
 */
const char* hid_string_descriptor[5] = {
    // array of pointer to string descriptors
    (char[]){0x09, 0x04},  // 0: is supported language is English (0x0409)
    "TinyUSB",             // 1: Manufacturer
    "TinyUSB Device",      // 2: Product
    "123456",              // 3: Serials, should use chip ID
    "Example HID interface",  // 4: HID
};

/**
 * @brief Configuration descriptor
 *
 * This is a simple configuration descriptor that defines 1 configuration and 1 HID interface
 */
static const uint8_t hid_configuration_descriptor[] = {
    // Configuration number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    // Interface number, string index, boot protocol, report descriptor len, EP In address, size & polling interval
    TUD_HID_DESCRIPTOR(0, 4, false, sizeof(hid_report_descriptor), 0x81, 16, 1),
};

/********* TinyUSB HID callbacks ***************/

// Invoked when received GET HID REPORT DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    // We use only one interface and one HID report descriptor, so we can ignore parameter 'instance'
    return hid_report_descriptor;
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;

    return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
}

/********* Application ***************/






void app_main(void)
{
    
    ESP_LOGI(TAG, "USB initialization");
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,
        .string_descriptor = hid_string_descriptor,
        .string_descriptor_count = sizeof(hid_string_descriptor) / sizeof(hid_string_descriptor[0]),
        .external_phy = false,
#if (TUD_OPT_HIGH_SPEED)
        .fs_configuration_descriptor = hid_configuration_descriptor, // HID configuration descriptor for full-speed and high-speed are the same
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
    int wasEnabled = 0;
    uint8_t buttonsDetect[sizeof(buttons)/sizeof(int)];
    uint8_t buttonsAfterDebounce[9];
    memset(buttonsDetect,0,9);
    memset(buttonsAfterDebounce,0,9);
    int ArrayPos = 0;
    uint64_t timerValue;
    int numberOfChecks = 0;
    while (1) {
        
        if(wasEnabled == 0)
            {
            gptimer_start(debouce_handler);
            wasEnabled = 1;
            }
            gptimer_get_raw_count(debouce_handler,&timerValue);
            
            
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
        memset(buttonsAfterDebounce,0,9);
        }
    }
}