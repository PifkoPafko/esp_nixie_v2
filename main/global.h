#ifndef __GLOBAL_H__
#define __GLOBAL_H__


#define NOTIFY_TASK(x, y) (xTaskNotify(x, y, eSetValueWithOverwrite))
#define NOTIFY_TASK_FROM_ISR(x, y) (xTaskNotifyFromISR(x, y, eSetValueWithOverwrite, NULL))

#endif