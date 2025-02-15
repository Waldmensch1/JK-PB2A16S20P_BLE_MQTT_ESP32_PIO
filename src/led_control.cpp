#include "led_control.h"

// Create a queue for LED states
QueueHandle_t ledQueue;

void ledTask(void *pvParameters) {
    LedState ledState;
    while (true) {
        if (xQueueReceive(ledQueue, &ledState, portMAX_DELAY) == pdTRUE) {
            switch (ledState) {
                case LED_OFF:
                    digitalWrite(LED_PIN, LOW);
                    break;
                case LED_ON:
                    digitalWrite(LED_PIN, HIGH);
                    break;
                case LED_BLINK_SLOW:
                    while (ledState == LED_BLINK_SLOW) {
                        digitalWrite(LED_PIN, HIGH);
                        vTaskDelay(1000 / portTICK_PERIOD_MS);
                        digitalWrite(LED_PIN, LOW);
                        vTaskDelay(1000 / portTICK_PERIOD_MS);
                        xQueuePeek(ledQueue, &ledState, 0);
                    }
                    break;
                case LED_BLINK_FAST:
                    while (ledState == LED_BLINK_FAST) {
                        digitalWrite(LED_PIN, HIGH);
                        vTaskDelay(200 / portTICK_PERIOD_MS);
                        digitalWrite(LED_PIN, LOW);
                        vTaskDelay(200 / portTICK_PERIOD_MS);
                        xQueuePeek(ledQueue, &ledState, 0);
                    }
                    break;
                case LED_DOUBLE_FLASH:
                    while (ledState == LED_DOUBLE_FLASH) {
                        digitalWrite(LED_PIN, HIGH);
                        vTaskDelay(100 / portTICK_PERIOD_MS);
                        digitalWrite(LED_PIN, LOW);
                        vTaskDelay(100 / portTICK_PERIOD_MS);
                        digitalWrite(LED_PIN, HIGH);
                        vTaskDelay(100 / portTICK_PERIOD_MS);
                        digitalWrite(LED_PIN, LOW);
                        vTaskDelay(1000 / portTICK_PERIOD_MS);
                        xQueuePeek(ledQueue, &ledState, 0);
                    }
                    break;
                case LED_FLASH:
                    while (ledState == LED_FLASH) {
                        digitalWrite(LED_PIN, HIGH);
                        vTaskDelay(500 / portTICK_PERIOD_MS);
                        digitalWrite(LED_PIN, LOW);
                        vTaskDelay(500 / portTICK_PERIOD_MS);
                        xQueuePeek(ledQueue, &ledState, 0);
                    }
                    break;
            }
        }
    }
}

void init_led() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

        // Create the LED queue
    ledQueue = xQueueCreate(10, sizeof(LedState));

    // Create the LED task
    xTaskCreate(ledTask, "LED Task", 2048, NULL, 1, NULL);
}

void set_led(LedState state) {
    xQueueSend(ledQueue, &state, portMAX_DELAY);
}