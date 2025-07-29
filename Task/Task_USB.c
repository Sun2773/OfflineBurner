#include "Task_USB.h"
#include "buzzer.h"
#include "hw_config.h"
#include "usb_pwr.h"

/**
 * @brief  USB任务处理函数
 * @note   100ms执行一次
 * @retval None
 */
void USB_Task(void) {
    static uint16_t usb_timeout = 0;
    if (USB_StateGet()) {
        if (usb_timeout < 20) {
            usb_timeout++;
        } else if (bDeviceState != CONFIGURED) {
            USB_Unload();
            Beep(300);
        }
    } else {
        usb_timeout = 0;
    }
}
