/*
 * Copyright 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "modem_if.h"
#include "bs_pc_base.h"
#include "bs_pc_base.h"
#include "bs_pc_2G4_utils.h"
#include "bs_rand_main.h"
#include <stdlib.h>
#include <stdio.h>

#ifdef BASIC
//compile as
/*  gcc -DBASIC test_ber.c ../modem_BLE_simple.c ../modem_BLE_simple_args.c    -I../../../ext_2G4_libPhyComv1/src/ -I../../../libRandv2/src/ -I../../../libPhyComv1/src/   -I../../../ext_2G4_phy_v1/src/ -I../../../libUtilv1/src/   ../../../../lib/libUtilv1.a ../../../../lib/libPhyComv1.a ../../../../lib/libRandv2.a ../../../../lib/lib2G4PhyComv1.a  -lm -o test_ber -std=c99 -Wall -pedantic -g
*/
//run as ./test_ber > BER.txt
//the results to be compared using matlab/BER_test.m

int main(int argc, char**argv){

  double SNR;
  double BER;

  void* ModemObj;
  p2G4_radioparams_t RxRadioParams;

  ModemObj = modem_init(argc-1, &argv[1], 0, 1);

  for ( SNR = -10; SNR < 30; SNR += 0.01 ) {
    uint32_t BER_i = modem_digital_perf_ber(ModemObj, &RxRadioParams, SNR);
    BER = ((double)BER_i)/RAND_PROB_1;
    fprintf(stdout,"%e %e\n",SNR,BER);
  }

  modem_delete(ModemObj);
}

#else

//compile as
/* gcc test_ber.c ../modem_BLE_simple.c ../modem_BLE_simple_args.c  -I../../../ext_2G4_libPhyComv1/src/ -I../../../libRandv2/src/ -I../../../libPhyComv1/src/   -I../../../ext_2G4_phy_v1/src/ -I../../../libUtilv1/src/   ../../../../lib/libUtilv1.a ../../../../lib/libPhyComv1.a ../../../../lib/libRandv2.a ../../../../lib/lib2G4PhyComv1.a  -lm -o test_ber -std=c99 -Wall -pedantic -g
*/
//run as ./test_ber > BER.txt
//the results to be compared using matlab/BER_test2.m

int main(int argc, char**argv){
  #define Ndevices 15

  uint device_nbr = 0;
  uint nbr_devices = Ndevices;
  void* ModemObj;
  double *RxPowers;
  tx_l_c_t Tx_List_container;
  uint desired_tx_nbr;
  p2G4_radioparams_t RxRadioParams;

  ModemObj = modem_init(argc-1, &argv[1], device_nbr, nbr_devices);

  Tx_List_container.tx_list = calloc(sizeof(tx_el_t), Ndevices);
  Tx_List_container.used = calloc(sizeof(tx_state_t), Ndevices);
  RxPowers = calloc(sizeof(double),Ndevices);

  desired_tx_nbr = 2;
  Tx_List_container.used[2] = 1;
  RxRadioParams.modulation = P2G4_MOD_BLE;
  p2G4_freq_from_d( 2450, 0, &RxRadioParams.center_freq );

  for ( double level = -110; level < -60; level += 0.2 ) {
    double OutputSNR;
    double Output_RSSI_power_level;

    RxPowers[2] = level;
    modem_analog_rx(ModemObj, &RxRadioParams, &OutputSNR, &Output_RSSI_power_level,
                    RxPowers, &Tx_List_container, desired_tx_nbr);

    uint32_t BER_i = modem_digital_perf_ber(ModemObj, &RxRadioParams, OutputSNR);
    double BER = ((double)BER_i)/RAND_PROB_1;
    fprintf(stdout,"%e %e %e\n",level, OutputSNR, BER);
  }

  modem_delete(ModemObj);

  free(Tx_List_container.tx_list);
  free(Tx_List_container.used);
  free(RxPowers);
}

#endif
