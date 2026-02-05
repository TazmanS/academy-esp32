#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define ADC_CHANNEL ADC_CHANNEL_3
#define ADC_UNIT_USED ADC_UNIT_1
#define ADC_ATTEN ADC_ATTEN_DB_12
#define ADC_BITWIDTH ADC_BITWIDTH_12

#define ADC_MAX_RAW 4095.0
#define ADC_MAX_MV 3300.0

static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t cali_handle;
static bool cali_enabled = false;

static void adc_init(void)
{
  adc_oneshot_unit_init_cfg_t unit_cfg = {
      .unit_id = ADC_UNIT_USED,
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc_handle));

  adc_oneshot_chan_cfg_t chan_cfg = {
      .bitwidth = ADC_BITWIDTH,
      .atten = ADC_ATTEN,
  };
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_cfg));

  adc_cali_curve_fitting_config_t cali_cfg = {
      .unit_id = ADC_UNIT_USED,
      .chan = ADC_CHANNEL,
      .atten = ADC_ATTEN,
      .bitwidth = ADC_BITWIDTH,
  };

  if (adc_cali_create_scheme_curve_fitting(&cali_cfg, &cali_handle) == ESP_OK)
  {
    cali_enabled = true;
  }
}

void app_main(void)
{
  adc_init();

  printf("\nRAW\tU_manual(mV)\tU_cali(mV)\tError(%%)\n");
  printf("----------------------------------------------------\n");

  while (1)
  {
    int raw;
    adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw);

    float u_manual = (raw / ADC_MAX_RAW) * ADC_MAX_MV;

    int u_cali = 0;
    float error = 0.0f;

    if (cali_enabled)
    {
      adc_cali_raw_to_voltage(cali_handle, raw, &u_cali);
      error = ((u_manual - u_cali) / u_cali) * 100.0f;
    }

    printf(
        "%4d\t%8.1f\t\t%8d\t\t%6.2f\n",
        raw,
        u_manual,
        u_cali,
        error);

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}