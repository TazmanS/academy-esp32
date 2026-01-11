#include "driver/gpio.h"
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED_PIN_1 GPIO_NUM_5
#define LED_PIN_2 GPIO_NUM_6
#define LED_PIN_3 GPIO_NUM_7

#define DELAY_1 200 * 1000
#define DELAY_2 500 * 1000
#define DELAY_3 1000 * 1000

int64_t previous_time_1 = 0;
int64_t previous_time_2 = 0;
int64_t previous_time_3 = 0;

bool led_state_1 = false;
bool led_state_2 = false;
bool led_state_3 = false;

extern "C" void app_main(void)
{
  gpio_config_t leds_config = {
      .pin_bit_mask = (1ULL << LED_PIN_1) | (1ULL << LED_PIN_2) | (1ULL << LED_PIN_3),
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE};

  gpio_config(&leds_config);

  gpio_set_level(LED_PIN_1, led_state_1);
  gpio_set_level(LED_PIN_2, led_state_2);
  gpio_set_level(LED_PIN_3, led_state_3);

  while (true)
  {
    int64_t time_now = esp_timer_get_time();

    if (time_now - previous_time_1 >= DELAY_1)
    {
      led_state_1 = !led_state_1;
      gpio_set_level(LED_PIN_1, led_state_1);
      previous_time_1 = time_now;
    }

    if (time_now - previous_time_2 >= DELAY_2)
    {
      led_state_2 = !led_state_2;
      gpio_set_level(LED_PIN_2, led_state_2);
      previous_time_2 = time_now;
    }

    if (time_now - previous_time_3 >= DELAY_3)
    {
      led_state_3 = !led_state_3;
      gpio_set_level(LED_PIN_3, led_state_3);
      previous_time_3 = time_now;
    }

    vTaskDelay(pdMS_TO_TICKS(1));
  }
}
