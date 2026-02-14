#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <driver/uart.h>
#include <esp_log.h>
#include <esp_timer.h>

static const char *TAG = "ESP32_UART_TOGGLE";

#define UART_PORT UART_NUM_1
#define UART_BAUD_RATE 115200
#define UART_TX_PIN GPIO_NUM_17
#define UART_RX_PIN GPIO_NUM_18

#define BUTTON_BOOT_PIN GPIO_NUM_0
#define LED_PIN GPIO_NUM_5

#define DEBOUNCE_MS (80 * 1000) // 80ms
#define CMD_STR "TOGGLE\n"

static int64_t s_last_press_ms = 0;
static int s_last_level = 1; // pull-up means idle HIGH

static void uart_init(void)
{
  const uart_config_t cfg = {
      .baud_rate = UART_BAUD_RATE,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_DEFAULT};

  ESP_ERROR_CHECK(uart_driver_install(UART_PORT, 1024, 0, 0, NULL, 0));
  ESP_ERROR_CHECK(uart_param_config(UART_PORT, &cfg));
  ESP_ERROR_CHECK(uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN,
                               UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

static void button_init(void)
{
  const gpio_config_t io_cfg = {
      .pin_bit_mask = 1ULL << BUTTON_BOOT_PIN,
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE};
  ESP_ERROR_CHECK(gpio_config(&io_cfg));
}

static void send_toggle_command(void)
{
  const int written = uart_write_bytes(UART_PORT, CMD_STR, (int)strlen(CMD_STR));
  ESP_LOGI(TAG, "Sent command (%d bytes): %s", written, "TOGGLE");
}

void led_config()
{
  const gpio_config_t io_cfg = {
      .pin_bit_mask = 1ULL << LED_PIN,
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE};
  ESP_ERROR_CHECK(gpio_config(&io_cfg));
}

void app_main(void)
{
  uart_init();
  button_init();
  led_config();
  uint8_t rx_buf[128];
  int state = 1;

  ESP_LOGI(TAG, "UART1 init: TX=%d RX=%d @%d",
           (int)UART_TX_PIN,
           (int)UART_RX_PIN, UART_BAUD_RATE);
  ESP_LOGI(TAG, "BOOT button on GPIO%d (active LOW). Press -> send TOGGLE",
           (int)BUTTON_BOOT_PIN);

  while (1)
  {
    const int level = gpio_get_level(BUTTON_BOOT_PIN);
    const int64_t t_ms = esp_timer_get_time();

    // detect falling edge (HIGH->LOW) with debounce
    if (s_last_level != level)
    {
      s_last_level = level;

      if (level == 0)
      { // active LOW
        if ((t_ms - s_last_press_ms) >= DEBOUNCE_MS)
        {
          s_last_press_ms = t_ms;
          ESP_LOGI(TAG, "Button pressed -> sending TOGGLE");
          send_toggle_command();
        }
      }
    }

    int len = uart_read_bytes(UART_PORT, rx_buf, sizeof(rx_buf) - 1, 10 / portTICK_PERIOD_MS);

    if (len > 0)
    {
      rx_buf[len] = 0;

      if (strstr((char *)rx_buf, "TOGGLE") != NULL)
      {
        ESP_LOGI(TAG, "Received data from STM32: %s", (char *)rx_buf);

        gpio_set_level(LED_PIN, state);
        state = !state;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}