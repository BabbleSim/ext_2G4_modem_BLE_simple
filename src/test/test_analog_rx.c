/*
 * Copyright 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "modem_if.h"
#include "bs_pc_base.h"
#include "bs_pc_base.h"
#include "bs_pc_2G4_utils.h"
#include <stdlib.h>
#include <stdio.h>

//compile as
/* gcc test_analog_rx.c ../modem_BLE_simple.c ../modem_BLE_simple_args.c  \
  -I../../../ext_2G4_libPhyComv1/src/ -I../../../libRandv2/src/ \
  -I../../../libPhyComv1/src/   -I../../../ext_2G4_phy_v1/src/ \
  -I../../../libUtilv1/src/   ../../../../lib/libUtilv1.a \
  ../../../../lib/libPhyComv1.a ../../../../lib/libRandv2.a ../../../../lib/lib2G4PhyComv1.a \
  ../../../../lib/libUtilv1.a ../../../../lib/libPhyComv1.a ../../../../lib/libRandv2.a  \
  -lm -o test_ana -std=c99 -Wall -pedantic -g
*/
//run as ./test_ana
//the results can be compared against the MATLAB reference (test_ana.m)

int main(int argc, char**argv){

#define Ndevices 15
  uint device_nbr = 0;
  uint nbr_devices = Ndevices;
  void* ModemObj;
  double RxPowers[Ndevices];
  tx_l_c_t Tx_List_container;
  uint desired_tx_nbr;
  p2G4_radioparams_t RxRadioParams;

  Tx_List_container.tx_list = calloc(sizeof(tx_el_t), Ndevices);
  Tx_List_container.used = calloc(sizeof(uint), Ndevices);

  ModemObj = modem_init(argc-1, &argv[1], device_nbr, nbr_devices);

  RxPowers[0] = 0;
  RxPowers[1] = -70; //1
  RxPowers[2] = -60; //desired
  RxPowers[3] = -60; //2
  RxPowers[4] = -50; //3
  RxPowers[5] = -80; //4
  RxPowers[6] = 0;
  RxPowers[7] = -80; //5
  RxPowers[9] = -70; //6
  RxPowers[10] = -60; //7

  desired_tx_nbr = 2;
  Tx_List_container.used[2] = 1;
  RxRadioParams.modulation = P2G4_MOD_BLE2M;
  p2G4_freq_from_d( 2450, 0, &RxRadioParams.center_freq );


  Tx_List_container.tx_list[1].tx_s.radio_params.modulation = P2G4_MOD_BLE;
  p2G4_freq_from_d( 2448e6, 0, &Tx_List_container.tx_list[1].tx_s.radio_params.center_freq );
  Tx_List_container.used[1] = 1;

  Tx_List_container.tx_list[3].tx_s.radio_params.modulation = P2G4_MOD_WHITENOISE1MHz;
  p2G4_freq_from_d( 2446e6, 0, &Tx_List_container.tx_list[3].tx_s.radio_params.center_freq );
  Tx_List_container.used[3] = 1;

  Tx_List_container.tx_list[4].tx_s.radio_params.modulation = P2G4_MOD_CWINTER;
  p2G4_freq_from_d( 2446e6, 0, &Tx_List_container.tx_list[4].tx_s.radio_params.center_freq );
  Tx_List_container.used[4] = 1;

  Tx_List_container.tx_list[5].tx_s.radio_params.modulation = P2G4_MOD_WHITENOISE4MHz;
  p2G4_freq_from_d( 2449e6, 0, &Tx_List_container.tx_list[5].tx_s.radio_params.center_freq );
  Tx_List_container.used[5] = 1;

  Tx_List_container.tx_list[7].tx_s.radio_params.modulation = P2G4_MOD_WHITENOISE2MHz;
  p2G4_freq_from_d( 2451e6, 0, &Tx_List_container.tx_list[7].tx_s.radio_params.center_freq );
  Tx_List_container.used[7] = 1;

  Tx_List_container.tx_list[9].tx_s.radio_params.modulation = P2G4_MOD_PROP4M; //should not contribute at all
  p2G4_freq_from_d( 2448e6, 0, &Tx_List_container.tx_list[9].tx_s.radio_params.center_freq );
  Tx_List_container.used[9] = 1;


  Tx_List_container.tx_list[10].tx_s.radio_params.modulation = P2G4_MOD_PROP2M; //should not contribute at all
  p2G4_freq_from_d( 2448e6, 0, &Tx_List_container.tx_list[10].tx_s.radio_params.center_freq );
  Tx_List_container.used[10] = 1;

  double OutputSNR;
  double Output_RSSI_power_level;

  modem_analog_rx(ModemObj, &RxRadioParams, &OutputSNR, &Output_RSSI_power_level,
                  RxPowers, &Tx_List_container, desired_tx_nbr);

  modem_delete(ModemObj);

  free(Tx_List_container.tx_list);
  free(Tx_List_container.used);

  fprintf(stdout,"SNR: %f\tRSSI:%f\n", OutputSNR, Output_RSSI_power_level);
}
