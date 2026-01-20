#include "esp_attr.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#define BUTTON_PIN GPIO_NUM_0

volatile int count = 0;
volatile int64_t last_interrupt_time = 0;
int debounce_delay = 50; // miliseconds
volatile bool button_pressed = false;
int debounce_button_state = 10; // miliseconds

bool button_state = false;

// task 1
// void IRAM_ATTR button_isr_handler(void *arg)
// {
//   count++;
// }

// task 2
// void IRAM_ATTR button_isr_handler(void *arg)
// {
//   button_pressed = true;
// }

// task 3
// void IRAM_ATTR button_isr_handler(void *arg)
// {
//   button_pressed = true;
// }

// task 4
void IRAM_ATTR button_isr_handler(void *arg)
{
}

void app_main(void)
{
  ESP_LOGI("MAIN", "START!!!");
  gpio_config_t button_config = {
      .pin_bit_mask = (1ULL << BUTTON_PIN),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_NEGEDGE};

  gpio_config(&button_config);

  gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
  gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, NULL);

  while (1)
  {
    // task 2
    // if (button_pressed)
    // {
    //   int64_t current_time = esp_timer_get_time() / 1000; // convert
    //   if (current_time - last_interrupt_time > debounce_delay)
    //   {
    //     count++;
    //     last_interrupt_time = current_time;
    //   }
    // }
    // button_pressed = false;

    // task 3
    // if (button_pressed)
    // {
    //   button_pressed = false;

    //   vTaskDelay(pdMS_TO_TICKS(50));

    //   int level = gpio_get_level(BUTTON_PIN);

    //   if (level == 0 && button_state == false)
    //   {
    //     count++;
    //     button_state = true;
    //     ESP_LOGI("BTN", "Pressed, count=%d", count);
    //   }

    //   if (level == 1)
    //   {
    //     button_state = false;
    //   }
    // }

    // task 4
    int64_t current_time = esp_timer_get_time() / 1000; // convert
    if (current_time - last_interrupt_time > debounce_button_state)
    {
      int level = gpio_get_level(BUTTON_PIN);

      if (level == 0 && button_state == false)
      {
        count++;
        button_state = true;
        last_interrupt_time = current_time;
        ESP_LOGI("BTN", "Pressed, count=%d", count);
      }

      if (level == 1)
      {
        button_state = false;
      }
    }

    // ESP_LOGI("COUNT", "Button pressed %d times", count);
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
