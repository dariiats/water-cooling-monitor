#include "pico/stdlib.h"

#include "FreeRTOS.h"
#include "task.h"

/* Library includes. */
#include <stdio.h>
#include "hardware/gpio.h"

#ifndef LED_DELAY_MS
#define LED_DELAY_MS 500
#endif

#define BUTTON_DELAY_MS 10

#define LED 2
#define BUTTON 3
#define	LED_BLINK_PRIORITY		(tskIDLE_PRIORITY + 1)
#define BUTTON_READ_PRIORITY    (tskIDLE_PRIORITY + 2)

volatile int counter = 0;

volatile unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50000;
int buttonState = 1;

void gpio_callback(uint gpio, uint32_t events) {
    (void)events;
    if (gpio == BUTTON) {
        lastDebounceTime = time_us_32();
    }
}

static void LED_Task(void *pvParameters) {
    (void)pvParameters;

    gpio_init(LED);
    gpio_set_dir(LED, GPIO_OUT);

    for (;;) {
        gpio_put(LED, 1);
        printf("[RTOS] LED TURN ON\n");
        vTaskDelay(pdMS_TO_TICKS(LED_DELAY_MS));
        gpio_put(LED, 0);
        printf("[RTOS] LED TURN OFF\n");
        vTaskDelay(pdMS_TO_TICKS(LED_DELAY_MS));
    }
}

static void BUTTON_READ_TASK(void *pvParameters) {
    (void)pvParameters;
    for (;;) {
        int reading = gpio_get(BUTTON);
        if ((time_us_32() - lastDebounceTime) > debounceDelay) {
            if (reading != buttonState) {
                buttonState = reading;
                counter++;
                printf("STATE: %d, COUNTER: %d\n", buttonState, counter);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(BUTTON_DELAY_MS));
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

    gpio_set_irq_enabled_with_callback(BUTTON, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    xTaskCreate(LED_Task, "LED", configMINIMAL_STACK_SIZE, NULL, LED_BLINK_PRIORITY, NULL);
    xTaskCreate(BUTTON_READ_TASK, "BUTTON", configMINIMAL_STACK_SIZE, NULL, BUTTON_READ_PRIORITY, NULL);
	vTaskStartScheduler();

	for (;;);
}
