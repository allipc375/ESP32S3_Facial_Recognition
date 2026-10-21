#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

extern "C" void app_main(void)
{
    while(true)
    {
        printf("ESP-IDF working!\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}