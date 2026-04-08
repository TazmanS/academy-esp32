#include <stdint.h>
#include <inttypes.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/ledc.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>
#include <esp_err.h>
#include <esp_log.h>

#define ADC_CHANNEL ADC_CHANNEL_8 // Pin 9
#define ADC_UNIT ADC_UNIT_1
#define ADC_ATTEN ADC_ATTEN_DB_12
#define ADC_BITWIDTH ADC_BITWIDTH_12

#define SERVO_PIN 14
#define SERVO_FREQ_HZ 50
#define SERVO_PERIOD_US (1000000 / SERVO_FREQ_HZ)
#define SERVO_RESOLUTION LEDC_TIMER_12_BIT
#define SERVO_MAX_DUTY ((1 << SERVO_RESOLUTION) - 1)
#define SERVO_TIMER LEDC_TIMER_0
#define SERVO_CHANNEL LEDC_CHANNEL_0

#define SUPERLOOP_DELAY_MS 100

typedef struct
{
  int min_deg;
  int max_deg;
  int min_pulse_us;
  int max_pulse_us;
  int light_min_mv;
  int light_max_mv;
} servo_calibration_t;

typedef struct
{
  adc_oneshot_unit_handle_t adc_handle;
  adc_cali_handle_t adc_cali_handle;
} app_context_t;

static const servo_calibration_t SERVO_CALIBRATION = {
    .min_deg = 10,
    .max_deg = 170,
    .min_pulse_us = 500,
    .max_pulse_us = 2500,
    .light_min_mv = 50,
    .light_max_mv = 3000,
};

static app_context_t app_context;

static const char *TAG = "Mini Project";

static void ldr_init(void)
{
  adc_oneshot_unit_init_cfg_t unit_config = {
      .unit_id = ADC_UNIT,
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_config, &app_context.adc_handle));

  adc_oneshot_chan_cfg_t channel_config = {
      .atten = ADC_ATTEN,
      .bitwidth = ADC_BITWIDTH,
  };
  ESP_ERROR_CHECK(adc_oneshot_config_channel(
      app_context.adc_handle,
      ADC_CHANNEL,
      &channel_config));

  adc_cali_curve_fitting_config_t cfg = {
      .unit_id = ADC_UNIT,
      .chan = ADC_CHANNEL,
      .atten = ADC_ATTEN,
      .bitwidth = ADC_BITWIDTH,
  };
  ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cfg, &app_context.adc_cali_handle));
}

static void servo_init(void)
{
  ledc_timer_config_t timer_config = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .timer_num = SERVO_TIMER,
      .duty_resolution = SERVO_RESOLUTION,
      .freq_hz = SERVO_FREQ_HZ,
      .clk_cfg = LEDC_AUTO_CLK};
  ESP_ERROR_CHECK(ledc_timer_config(&timer_config));

  ledc_channel_config_t channel_config = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = SERVO_CHANNEL,
      .timer_sel = SERVO_TIMER,
      .intr_type = LEDC_INTR_DISABLE,
      .gpio_num = SERVO_PIN,
      .duty = 0};
  ESP_ERROR_CHECK(ledc_channel_config(&channel_config));
}

static int val_clamp(int value, int val_min, int val_max)
{
  if (value < val_min)
    return val_min;
  if (value > val_max)
    return val_max;
  return value;
}

static int map_int(int value, int in_min, int in_max, int out_min, int out_max)
{
  if (in_min == in_max)
    return out_max;

  value = val_clamp(value, in_min, in_max);

  return out_min + (value - in_min) * (out_max - out_min) / (in_max - in_min);
}

static uint32_t pulse_us_to_duty(int pulse_us)
{
  return ((uint32_t)pulse_us * SERVO_MAX_DUTY) / SERVO_PERIOD_US;
}

static int light_mv_to_servo_angle(int light_mv)
{
  return map_int(
      light_mv,
      SERVO_CALIBRATION.light_min_mv,
      SERVO_CALIBRATION.light_max_mv,
      SERVO_CALIBRATION.min_deg,
      SERVO_CALIBRATION.max_deg);
}

static int servo_angle_to_pulse_us(int angle_deg)
{
  return map_int(
      angle_deg,
      SERVO_CALIBRATION.min_deg,
      SERVO_CALIBRATION.max_deg,
      SERVO_CALIBRATION.min_pulse_us,
      SERVO_CALIBRATION.max_pulse_us);
}

static void set_servo_from_light_mv(int light_mv)
{
  int angle_deg = light_mv_to_servo_angle(light_mv);
  int pulse_us = servo_angle_to_pulse_us(angle_deg);
  uint32_t duty = pulse_us_to_duty(pulse_us);

  ESP_LOGI(TAG,
           "Light=%d mV Angle=%d deg Pulse=%d us Duty=%" PRIu32,
           light_mv,
           angle_deg,
           pulse_us,
           duty);

  ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, SERVO_CHANNEL, duty));
  ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, SERVO_CHANNEL));
}

static int read_light_mv(void)
{
  int raw = 0;
  int light_mv = 0;

  ESP_ERROR_CHECK(adc_oneshot_read(app_context.adc_handle, ADC_CHANNEL, &raw));
  ESP_ERROR_CHECK(adc_cali_raw_to_voltage(app_context.adc_cali_handle, raw, &light_mv));

  ESP_LOGI(TAG, "Raw ADC=%d Voltage=%d mV", raw, light_mv);
  return light_mv;
}

void app_main(void)
{
  ldr_init();
  servo_init();

  while (1)
  {
    int light_mv = read_light_mv();
    set_servo_from_light_mv(light_mv);
    vTaskDelay(pdMS_TO_TICKS(SUPERLOOP_DELAY_MS));
  }
}