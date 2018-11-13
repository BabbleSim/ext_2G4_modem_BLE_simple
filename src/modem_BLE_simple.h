/*
 * Copyright 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef BS_MODEM_BLE_SIMPLE_H
#define BS_MODEM_BLE_SIMPLE_H

#include "modem_BLE_simple_args.h"

typedef struct {
  uint32_t nbr_devices;
  mo_simple_args_t args;
  double device_RSSI_offsetdB; //Average measurement offset (for all channels)
} m_simple_status_t;

#endif
