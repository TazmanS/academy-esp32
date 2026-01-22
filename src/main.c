#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

#define FAN_PIN GPIO_NUM_5

#define FAN_PERIODIC 10 // 10 seconds
#define FAN_RUNTIME 5   // 5 seconds

esp_timer_handle_t fan_runtime_timer;
esp_timer_handle_t fan_periodic_timer;

const char *TAG = "FAN";

void fan_turn_off()
{
  ESP_LOGI(TAG, "Turn off fan");
  gpio_set_level(FAN_PIN, 0);
}

void fan_turn_on()
{
  ESP_LOGI(TAG, "Turn on fan");
  gpio_set_level(FAN_PIN, 1);
}

void fan_runtime_callback(void *arg)
{
  ESP_LOGI(TAG, "fan_runtime_callback");
  fan_turn_off();
}

void fan_periodic_callback(void *arg)
{
  ESP_LOGI(TAG, "fan_periodic_callback");
  fan_turn_on();

  esp_timer_stop(fan_runtime_timer);
  esp_timer_start_once(fan_runtime_timer, FAN_RUNTIME * 1000000LL);
}

void app_main(void)
{
  ESP_LOGI(TAG, "START PROGRAMM");
  gpio_config_t fan_config = {
      .pin_bit_mask = (1ULL << FAN_PIN),
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  gpio_config(&fan_config);

  fan_turn_off();

  esp_timer_create_args_t fan_runtime_config = {
      .callback = &fan_runtime_callback,
      .name = "fan_runtime_timer",
  };
  esp_timer_create(&fan_runtime_config, &fan_runtime_timer);

  esp_timer_create_args_t fan_periodic_config = {
      .callback = &fan_periodic_callback,
      .name = "fan_periodic_timer",
  };
  esp_timer_create(&fan_periodic_config, &fan_periodic_timer);

  esp_timer_start_periodic(fan_periodic_timer, FAN_PERIODIC * 1000000LL);
}
