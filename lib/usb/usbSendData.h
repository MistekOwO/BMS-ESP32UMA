#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "tinyusb.h"
#include "class/hid/hid_device.h"

typedef struct __attribute__((packed)) {
    int8_t x;
    int8_t y;
    uint16_t buttons;
} gamepad_report_t;
uint16_t GpioPinToKeyCode(uint8_t pin)
{
switch(pin)
{
    //WhiteKeys
    case 35:
    return 0;
    //return HID_KEY_X;
    break;
    case 34:
    return 1;
    //return HID_KEY_C;
    break;
    case 13:
    return 2;
    //return HID_KEY_V;
    break;
    case 42:
    return 3;
    //return HID_KEY_B;
    break;

    //BlackKeys
    case 5:
    return 4;
    //return HID_KEY_D;
    break;
    case 16:
    return 5;
    //return HID_KEY_F;
    break;
    case 17:
    return 6;
    //return HID_KEY_G;
    break;

    //FunctionKeys
    case 3:
    return 7;
    //return HID_KEY_J;
    break;
    case 36:
    return 8;
    //return HID_KEY_K;
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
    for(int i=0;i<9;i++)
    {
        report.buttons |= (1<< GpioPinToKeyCode(buttons[i]));
    }
    tud_hid_report(0,&report,sizeof(report));
}
}




