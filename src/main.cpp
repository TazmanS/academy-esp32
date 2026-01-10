#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_timer.h"

const gpio_num_t RELE_PIN = GPIO_NUM_5;
const gpio_num_t READ_RELE_PIN = GPIO_NUM_7;

volatile int64_t time_now = 0;
volatile int64_t total_time = 0;
volatile int count = 0;

void IRAM_ATTR rele_isr_handler(void *arg)
{
  total_time += esp_timer_get_time() - time_now;
  count++;
}

extern "C" void app_main(void)
{
  esp_log_level_set("*", ESP_LOG_INFO);

  gpio_config_t rele_config = {
      .pin_bit_mask = (1ULL << RELE_PIN),
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE};

  gpio_config(&rele_config);

  gpio_config_t rele_read_config = {
      .pin_bit_mask = (1ULL << READ_RELE_PIN),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_NEGEDGE};

  gpio_config(&rele_read_config);

  gpio_install_isr_service(0);
  gpio_isr_handler_add(READ_RELE_PIN, rele_isr_handler, nullptr);

  while (true)
  {
    time_now = esp_timer_get_time();
    // RELE ON
    gpio_set_level(RELE_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(1000));

    // RELE OFF
    gpio_set_level(RELE_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI("RELE", "Time: %lld us", total_time / count);
  }
}
