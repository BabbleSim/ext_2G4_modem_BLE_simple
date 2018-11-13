/*
 * Copyright 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef M_BLE_SIMPLE_ARGS_H
#define M_BLE_SIMPLE_ARGS_H

#include "bs_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
  uint32_t Sync_prob;

  double NFigure_ana; //Noise figure
  double NFigure_extra; //Extra noise figure before the digital demodulation
  double NFloor_dig; //Noise floor seen in the digital demodulation

  double RSSI_offsetdB_std;    //std deviation of the residual offset (for all channels) in this device RSSI measurements
  double RSSI_meas_noisedB_std;//std deviation of the RSSI mesaurement error
} mo_simple_args_t;

void modem_simple_argparse(int argc, char *argv[], uint d, mo_simple_args_t *args);

#ifdef __cplusplus
}
#endif

#endif
