#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"

#define LED_GPIO 5
#define LEDC_TIMER_ID LEDC_TIMER_0
#define LEDC_CHANNEL_ID LEDC_CHANNEL_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE

#define PWM_FREQUENCY_HZ 5000
#define PWM_RESOLUTION LEDC_TIMER_12_BIT

#define MAX_DUTY ((1 << PWM_RESOLUTION) - 1)

static const int LED_STEP = 100;

static void pwm_led_init(void)
{
  ledc_timer_config_t timer_config = {
      .speed_mode = LEDC_MODE,
      .timer_num = LEDC_TIMER_ID,
      .duty_resolution = PWM_RESOLUTION,
      .freq_hz = PWM_FREQUENCY_HZ,
      .clk_cfg = LEDC_AUTO_CLK};
  ESP_ERROR_CHECK(ledc_timer_config(&timer_config));

  ledc_channel_config_t channel_config = {
      .speed_mode = LEDC_MODE,
      .channel = LEDC_CHANNEL_ID,
      .timer_sel = LEDC_TIMER_ID,
      .intr_type = LEDC_INTR_DISABLE,
      .gpio_num = LED_GPIO,
      .duty = 0};
  ESP_ERROR_CHECK(ledc_channel_config(&channel_config));
}

void app_main(void)
{
  pwm_led_init();

  while (1)
  {
    for (int duty = 0; duty <= MAX_DUTY; duty += LED_STEP)
    {
      ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_ID, duty);
      ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_ID);
      vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Fade down
    for (int duty = MAX_DUTY; duty >= 0; duty -= LED_STEP)
    {
      ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_ID, duty);
      ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_ID);
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
}