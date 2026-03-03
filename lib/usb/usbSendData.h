#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "tinyusb.h"
#include "class/hid/hid_device.h"
#include "config.h"

typedef struct __attribute__((packed)) {
    int8_t x;
    int8_t y;
    uint16_t buttons;
} gamepad_report_t;
uint16_t GpioPinToKeyCode(uint8_t pin)
{
switch(pin)
{
    case W1Button:
    return 0;
    break;
    case W2Button:
    return 1;
    break;
    case W3Button:
    return 2;
    break;
    case W4Button:
    return 3;
    break;

    //BlackKeys
    case B1Button:
    return 4;
    break;
    case B2Button:
    return 5;
    break;
    case B3Button:
    return 6;
    break;

    //FunctionKeys
    case F1Button:
    return 7;
    break;
    case F2Button:
    return 8;
    break;
    case F3Button:
    return 9;
    break;
    case F4Button:
    return 10; 
    break;
    default:
    return 656565;
    break;

}
}



void sendControllerInputs(uint8_t buttons[],int encoderValue)
{
    gamepad_report_t report;
    report.x = 0;
    report.y = encoderValue/5;
    report.buttons = 0;
if(tud_hid_ready())
{
    for(int i=0;i<11;i++)
    {
        report.buttons |= (1<< GpioPinToKeyCode(buttons[i]));
    }
    tud_hid_report(0,&report,sizeof(report));
}
}




