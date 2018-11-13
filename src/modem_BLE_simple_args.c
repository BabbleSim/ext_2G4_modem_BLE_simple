/*
 * Copyright 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>
#include <stddef.h>
#include "bs_tracing.h"
#include "bs_oswrap.h"
#include "bs_rand_main.h"
#include "modem_BLE_simple_args.h"
#include "bs_cmd_line_typical.h"

static char library_name[] = "Modem BLE_simple";

void modem_print_post_help() {
  fprintf(stdout,"Simplistic model of a BLE modem\n\n"
"This modem model includes the following configurable Rx noise sources:\n"
" * Analog noise figure: Noise figure of the analog part (4dB by default)\n"
" * Extra noise figure: Extra noise figure that may be introduced after the power\n"
"                measurements (2dB by default)\n"
" * Noise floor: Noise floor of the modem. For example due to the digital dynamic\n"
"                range. (-35dB by default). This can be seen as a SNR limit\n"
"\n"
"This modem RSSI measurements include 2 types of errors.\n"
" * RSSI_offset: (1) Equivalent to a RSSI production calibration offset. A value\n"
"                will be drawn during initialization from a N(0, RSSI_offset_std),\n"
"                and will be constant during runtime and for the whole band\n"
" * RSSI measurement noise: (1.5) The error in each RSSI measurement is modeled as a\n"
"                N(RSSI_offset, RSSI_meas_noisedB_std)\n"
  );
}

static mo_simple_args_t *args_g;
static uint device_nbr_g;
static double per;

static void cmd_per_found(char * argv, int offset) {
  if ((per < 0) || (per > 1)) {
    bs_trace_error("modem: %i: (BLE_simple) args: PER shall be between 0 and 1 '%s'\n", device_nbr_g, per, argv);
  }
  args_g->Sync_prob = (1.0-per)*(double)RAND_PROB_1;
  bs_trace_raw(9,"modem: %i: (BLE_simple) sync probability (per) set to %u/%u (%e)\n", device_nbr_g, args_g->Sync_prob, RAND_PROB_1, per);
}

/**
 * Check the arguments provided in the command line: set args based on it
 * or defaults, and check they are correct
 */
void modem_simple_argparse(int argc, char *argv[], uint d, mo_simple_args_t *args)
{
  args_g = args;
  device_nbr_g = d;
  bs_args_struct_t args_struct[] = {
    /*manual,mandatory,switch,option,name , type, destination,  callback,       description */
    { false , false  , false, "PER", "per", 'f', (void*)&per, cmd_per_found, "Synchronization packet error rate (note that this is in excess of any synchronization errors due to bit errors"},
    { false , false  , false, "RSSI_offset_std", "RSSI_offset", 'f', (void*)&args->RSSI_offsetdB_std, NULL, "Standard deviation of the device RSSI measurement offset"},
    { false , false  , false, "RSSI_meas_noisedB_std", "RSSI_meas_noise", 'f', (void*)&args->RSSI_meas_noisedB_std, NULL, "Standard deviation of the device RSSI measurement noise"},
    { false , false  , false, "NFAna", "nfig", 'f', (void*)&args->NFigure_ana, NULL, "Noise figure"},
    { false , false  , false, "NFExtra", "nfigext", 'f', (void*)&args->NFigure_extra, NULL, "Extra noise figure up to the digital demodulation"},
    { false , false  , false, "NFloor",  "nfloor", 'f', (void*)&args->NFloor_dig, NULL, "Digital noise floor"},
    ARG_TABLE_ENDMARKER
  };

  //set defaults:
  args->Sync_prob = RAND_PROB_1;
  args->RSSI_offsetdB_std = 1.0;
  args->RSSI_meas_noisedB_std = 1.5;
  args->NFigure_ana = 4;
  args->NFigure_extra = 2;
  args->NFloor_dig  = -35; //35 dB noise floor by default

  char trace_prefix[50]; //this variable won't be used as soon as we get out of this function
  snprintf(trace_prefix,50, "modem: %i: (simple) ",d);

  bs_args_override_exe_name(library_name);
  bs_override_post_help(modem_print_post_help);
  bs_args_set_trace_prefix(trace_prefix);
  bs_args_parse_all_cmd_line(argc, argv, args_struct);
}
