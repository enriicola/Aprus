// SPDX-License-Identifier: MIT
// Copyright 2023 aprus

#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct
{
  pin_t pin;
  float ph;
} chip_state_t;

void chip_timer_callback(void *data)
{
  chip_state_t *chip = (chip_state_t *)data;
  float ph = attr_read(chip->ph);
  float volts = 5 * (ph / 4096.0);
  pin_dac_write(chip->pin, volts);
}

void chip_init()
{
  chip_state_t *chip = (chip_state_t *)malloc(sizeof(chip_state_t));
  chip->ph = attr_init("ph", 7.0);
  chip->pin = pin_init("A0", ANALOG);

  const timer_config_t config = {
      .callback = chip_timer_callback,
      .user_data = chip,
  };

  timer_t timer_id = timer_init(&config);
  timer_start(timer_id, 1000, true);
}
