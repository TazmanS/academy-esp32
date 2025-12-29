#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_attr.h"

enum class LedState : uint8_t
{
  LED_OFF = 0,
  LED_ON = 1,
};

enum class LedMode : uint8_t
{
  LED_MODE_OFF = 0,
  LED_MODE_ON = 1,
  LED_MODE_BLINK = 2,
};

class BlinkConfig
{
public:
  static constexpr int64_t BlinkPeriod = 1000000;   // 1 second
  static constexpr int64_t DebouncePeriod = 200000; // 200 ms
};

class Led
{
public:
  explicit Led(gpio_num_t pin) : _pin(pin)
  {
  }

  void init()
  {
    gpio_config_t io_config = {
        .pin_bit_mask = (1ULL << _pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_config);

    set(LedState::LED_OFF);
  }

  void set(LedState state)
  {
    gpio_set_level(_pin, (state == LedState::LED_ON) ? 1 : 0);
  }

  void work()
  {
    switch (_led_mode)
    {
    case LedMode::LED_MODE_OFF:
      set(LedState::LED_OFF);
      break;
    case LedMode::LED_MODE_ON:
      set(LedState::LED_ON);
      break;
    case LedMode::LED_MODE_BLINK:
      if (esp_timer_get_time() - _last_time >= _delay)
      {
        _last_time = esp_timer_get_time();
        _led_state = (_led_state == LedState::LED_OFF) ? LedState::LED_ON : LedState::LED_OFF;
        set(_led_state);
      }
      break;
    default:
      break;
    }
  }

  void next_mode()
  {
    switch (_led_mode)
    {
    case LedMode::LED_MODE_OFF:
      _led_mode = LedMode::LED_MODE_ON;
      break;
    case LedMode::LED_MODE_ON:
      _led_mode = LedMode::LED_MODE_BLINK;
      break;
    case LedMode::LED_MODE_BLINK:
      _led_mode = LedMode::LED_MODE_OFF;
      break;
    default:
      break;
    }
  }

private:
  gpio_num_t _pin;
  int64_t _last_time = 0;
  int64_t _delay = BlinkConfig::BlinkPeriod;
  LedState _led_state = LedState::LED_OFF;
  LedMode _led_mode = LedMode::LED_MODE_BLINK;
};

volatile bool button_pressed = false;

void IRAM_ATTR button_isr(void *arg)
{
  button_pressed = true;
};

extern "C" void app_main(void)
{
  gpio_config_t button_config = {
      .pin_bit_mask = (1ULL << GPIO_NUM_15),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_NEGEDGE,
  };

  gpio_config(&button_config);

  gpio_install_isr_service(0);
  gpio_isr_handler_add(GPIO_NUM_15, button_isr, nullptr);

  int64_t last_press_time = 0;

  Led led(GPIO_NUM_5);
  led.init();

  while (1)
  {
    led.work();

    if (button_pressed)
    {
      button_pressed = false;

      int64_t now = esp_timer_get_time();
      if (now - last_press_time >= BlinkConfig::DebouncePeriod)
      {
        last_press_time = now;
        led.next_mode();
      }
    }
  };
}
