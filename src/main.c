#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_attr.h"

typedef enum
{
  TRAFFIC_RED_IDLE,
  TRAFFIC_RED_YELLOW,
  TRAFFIC_GREEN_IDLE,
  TRAFFIC_GREEN_BLINK,
  TRAFFIC_YELLOW_IDLE,
  TRAFFIC_YELLOW_BLINK
} traffic_light_state_t;

#define TRAFFIC_PIN_RED GPIO_NUM_7
#define TRAFFIC_PIN_YELLOW GPIO_NUM_6
#define TRAFFIC_PIN_GREEN GPIO_NUM_5
#define BUTTON_PIN GPIO_NUM_0

#define BLINK_DELAY_MS 500
#define TRAFFIC_RED_DURATION_MS 5000
#define TRAFFIC_RED_YELLOW_MS 2000
#define TRAFFIC_GREEN_DURATION_MS 5000
#define TRAFFIC_GREEN_BLINK_MS 3000
#define TRAFFIC_YELLOW_DURATION_MS 2000
#define DEBOUNCE_TIME_MS 200

static traffic_light_state_t current_state = TRAFFIC_RED_IDLE;
static uint64_t state_start_time = 0;
volatile bool emergency_mode = false;
volatile uint64_t last_button_isr_time = 0;

static uint64_t now_ms(void)
{
  return esp_timer_get_time() / 1000;
}

static void set_lights(int red, int yellow, int green)
{
  gpio_set_level(TRAFFIC_PIN_RED, red);
  gpio_set_level(TRAFFIC_PIN_YELLOW, yellow);
  gpio_set_level(TRAFFIC_PIN_GREEN, green);
}

static void IRAM_ATTR button_isr_handler(void *arg)
{
  uint64_t now = esp_timer_get_time() / 1000;

  if (now - last_button_isr_time < DEBOUNCE_TIME_MS)
    return;

  last_button_isr_time = now;

  emergency_mode = !emergency_mode;
}

void app_main(void)
{
  gpio_config_t lights_io = {
      .pin_bit_mask = (1ULL << TRAFFIC_PIN_RED) |
                      (1ULL << TRAFFIC_PIN_YELLOW) |
                      (1ULL << TRAFFIC_PIN_GREEN),
      .mode = GPIO_MODE_OUTPUT,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  gpio_config(&lights_io);

  gpio_config_t btn_io = {
      .pin_bit_mask = (1ULL << BUTTON_PIN),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_NEGEDGE};
  gpio_config(&btn_io);

  gpio_install_isr_service(0);
  gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, NULL);

  state_start_time = now_ms();

  while (1)
  {
    uint64_t elapsed = now_ms() - state_start_time;

    if (emergency_mode)
    {
      int blink = (elapsed / BLINK_DELAY_MS) % 2;

      gpio_set_level(TRAFFIC_PIN_RED, 0);
      gpio_set_level(TRAFFIC_PIN_GREEN, 0);
      gpio_set_level(TRAFFIC_PIN_YELLOW, blink);

      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }

    switch (current_state)
    {
    case TRAFFIC_RED_IDLE:
      set_lights(1, 0, 0);
      if (elapsed >= TRAFFIC_RED_DURATION_MS)
      {
        current_state = TRAFFIC_RED_YELLOW;
        state_start_time = now_ms();
      }
      break;

    case TRAFFIC_RED_YELLOW:
      set_lights(1, 1, 0);
      if (elapsed >= TRAFFIC_RED_YELLOW_MS)
      {
        current_state = TRAFFIC_GREEN_IDLE;
        state_start_time = now_ms();
      }
      break;

    case TRAFFIC_GREEN_IDLE:
      set_lights(0, 0, 1);
      if (elapsed >= TRAFFIC_GREEN_DURATION_MS)
      {
        current_state = TRAFFIC_GREEN_BLINK;
        state_start_time = now_ms();
      }
      break;

    case TRAFFIC_GREEN_BLINK:
    {
      int blink = (elapsed / BLINK_DELAY_MS) % 2;
      set_lights(0, 0, blink);

      if (elapsed >= TRAFFIC_GREEN_BLINK_MS)
      {
        current_state = TRAFFIC_YELLOW_IDLE;
        state_start_time = now_ms();
      }
      break;
    }

    case TRAFFIC_YELLOW_IDLE:
      set_lights(0, 1, 0);
      if (elapsed >= TRAFFIC_YELLOW_DURATION_MS)
      {
        current_state = TRAFFIC_RED_IDLE;
        state_start_time = now_ms();
      }
      break;

    default:
      current_state = TRAFFIC_RED_IDLE;
      state_start_time = now_ms();
      break;
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
