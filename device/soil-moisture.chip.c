#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  pin_t moisture_pin;
  pin_t temperature_pin;
  float moisture;
  float temperature;
} chip_data_t;

void chip_timer_callback(void *data) 
{
  chip_data_t *chip_data = (chip_data_t*)data;

  // Read moisture value and convert to voltage
  float moisture = attr_read(chip_data->moisture);
  float moisture_volts = 5 * (moisture / 4096.0);
  pin_dac_write(chip_data->moisture_pin, moisture_volts);

  // Read temperature value and convert to voltage
  float temperature = attr_read(chip_data->temperature);
  float temp_volts = 5 * (temperature / 4096.0);
  pin_dac_write(chip_data->temp_pin, temp_volts);
}

void chip_init()
{
  chip_data_t *chip_data = (chip_data_t *)malloc(sizeof(chip_data_t));
  chip_data->moisture = attr_init("moisture", 2910.0);
  chip_data->temperature = attr_init("temperature", 2000.0);
  chip_data->moisture_pin = pin_init("A0", ANALOG);
  chip_data->temp_pin = pin_init("A1", ANALOG);

  const timer_config_t config =
      {
          .callback = chip_timer_callback,
          .user_data = chip_data,
      };

  timer_t timer_id = timer_init(&config);
  timer_start(timer_id, 1000, true);
}