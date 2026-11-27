#include "pico/stdlib.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Library includes. */
#include <stdio.h>
#include "hardware/gpio.h"

#ifndef LED_DELAY_MS
#define LED_DELAY_MS 500
#endif

#define LED 2
#define BUTTON 3
#define	LED_BLINK_PRIORITY		(tskIDLE_PRIORITY + 1)
#define BUTTON_READ_PRIORITY    (tskIDLE_PRIORITY + 2)

typedef struct {
    uint8_t buttonValue;
    uint32_t time;
} buttonInfo;

int counter = 0;

int buttonState = 1;

TaskHandle_t xButtonHandle = NULL;
QueueHandle_t xQueue;

void gpio_callback(uint gpio, uint32_t events) {
    (void)events;
    if (gpio == BUTTON) {
        BaseType_t xHigherPriorityTaskWoken;
        xHigherPriorityTaskWoken = pdFALSE;

        vTaskNotifyGiveFromISR(xButtonHandle, &xHigherPriorityTaskWoken);

        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

static void LED_Task(void *pvParameters) {
    (void)pvParameters;

    BaseType_t xStatus;
    buttonInfo button_info;

    bool led_on = false;
    bool paused = false;
    uint32_t pause_start = 0;
    uint32_t pause_stop = 0;

    gpio_init(LED);
    gpio_set_dir(LED, GPIO_OUT);

    for (;;) {
        TickType_t timeout = paused ? portMAX_DELAY : pdMS_TO_TICKS(LED_DELAY_MS);
        xStatus = xQueueReceive(xQueue, &button_info, timeout);
        if (xStatus == pdPASS) {
            paused = (button_info.buttonValue == 0);
            if (paused) {
                led_on = false;
                gpio_put(LED, led_on);
                printf("[RTOS] LED PAUSED\n");
                pause_start = button_info.time;
            } else {
                pause_stop = button_info.time;
                printf("Pause duration: %lu\n", (pause_stop - pause_start) / 1000);
                pause_start = 0;
                pause_stop = 0;
            }
        } else {
            led_on = !led_on;
            gpio_put(LED, led_on);
            printf("[RTOS] LED TURN %s\n", led_on ? "ON" : "OFF");
        }
    }
}

static void BUTTON_READ_TASK(void *pvParameters) {
    (void)pvParameters;

    BaseType_t xStatus;
    const TickType_t xDebounceWindow = pdMS_TO_TICKS(50);
    buttonInfo button_info;

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        while (ulTaskNotifyTake(pdTRUE, xDebounceWindow) != 0);

        int reading = gpio_get(BUTTON);
        if (reading != buttonState) {
            buttonState = reading;
            counter++;
            printf("STATE: %d, COUNTER: %d\n", buttonState, counter);

            button_info.buttonValue = buttonState;
            button_info.time = time_us_32();
            xStatus = xQueueSend(xQueue, &button_info, 0);
            if(xStatus != pdPASS) {
                printf("Could not send to the queue.\r\n");
            }
        }
    }
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *task_name) {
    (void)task;
    panic("Stack overflow in task: %s\n", task_name);
}

void vApplicationMallocFailedHook(void) {
    panic("FreeRTOS heap allocation failed\n");
}

int main(void) {
    stdio_init_all();
    while (!stdio_usb_connected()) {
        sleep_ms(250);
    }

    gpio_init(BUTTON);
    gpio_set_dir(BUTTON, GPIO_IN);
    gpio_pull_up(BUTTON);

    xQueue = xQueueCreate(5, sizeof(buttonInfo));

    if(xQueue != NULL) {
        xTaskCreate(LED_Task, "LED", configMINIMAL_STACK_SIZE, NULL, LED_BLINK_PRIORITY, NULL);
        xTaskCreate(BUTTON_READ_TASK, "BUTTON", configMINIMAL_STACK_SIZE, NULL, BUTTON_READ_PRIORITY, &xButtonHandle);

        gpio_set_irq_enabled_with_callback(BUTTON, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

        vTaskStartScheduler();
    }
	for (;;);
}
