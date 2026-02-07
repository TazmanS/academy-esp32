#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define LED_PIN GPIO_NUM_5
#define LIGHT_ON_THRESHOLD 1800
#define LIGHT_OFF_THRESHOLD 2200
bool isLightUp = false;

#define ADC_PIN GPIO_NUM_4
#define ADC_CHANEL ADC_CHANNEL_3
#define ADC_UNIT ADC_UNIT_1
#define ADC_ATTEN ADC_ATTEN_DB_12
#define ADC_BITWIDTH ADC_BITWIDTH_12

#define TAG "ADC_SMA"

#define SMA_WINDOW_SIZE 16

static adc_oneshot_unit_handle_t adc_handle;

static int sma_buffer[SMA_WINDOW_SIZE];
static int sma_index = 0;
static int sma_count = 0;
static int sma_sum = 0;

static int sma_add_sample(int new_sample)
{
  if (sma_count == SMA_WINDOW_SIZE)
  {
    sma_sum -= sma_buffer[sma_index];
  }
  else
  {
    sma_count++;
  }

  sma_buffer[sma_index] = new_sample;
  sma_sum += new_sample;

  sma_index++;
  if (sma_index >= SMA_WINDOW_SIZE)
  {
    sma_index = 0;
  }

  return sma_sum / sma_count;
}

void app_main(void)
{
  gpio_config_t io_config = {
      .pin_bit_mask = (1ULL << LED_PIN),
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE};
  gpio_config(&io_config);

  adc_oneshot_unit_init_cfg_t adc_unit_conf = {
      .unit_id = ADC_UNIT,
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_unit_conf, &adc_handle));

  adc_oneshot_chan_cfg_t adc_chan_cong = {
      .bitwidth = ADC_BITWIDTH,
      .atten = ADC_ATTEN,
  };
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANEL, &adc_chan_cong));

  while (1)
  {
    int raw_adc_value = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANEL, &raw_adc_value));

    int averaged_value = sma_add_sample(raw_adc_value);

    ESP_LOGI("ADC", "SMA data %d", averaged_value);

    if (!isLightUp && averaged_value < LIGHT_ON_THRESHOLD)
    {
      isLightUp = true;
      gpio_set_level(LED_PIN, 1);
      ESP_LOGI("LED", "Turn ON");
    }
    else if (isLightUp && averaged_value > LIGHT_OFF_THRESHOLD)
    {
      isLightUp = false;
      gpio_set_level(LED_PIN, 0);
      ESP_LOGI("LED", "Turn OFF");
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
