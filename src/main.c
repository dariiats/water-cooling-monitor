#include "pico/stdlib.h"

#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

#ifndef LED_DELAY_MS
#define LED_DELAY_MS 500
#endif

#define LED 2

// Perform initialisation
void led_init(void) {
    gpio_init(LED);
    gpio_set_dir(LED, GPIO_OUT);
}

// Turn the led on or off
void set_led(bool led_on) {
    gpio_put(LED, led_on);
}

int main(void) {
    stdio_init_all();
    while (!stdio_usb_connected()) {
        sleep_ms(250);
    }
    led_init();
    while (true) {
        set_led(true);
        printf("LED TURN ON\n");
        sleep_ms(LED_DELAY_MS);
        set_led(false);
        printf("LED TURN OFF\n");
        sleep_ms(LED_DELAY_MS);
    }
}
