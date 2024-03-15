/*
 * Copyright 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "bs_types.h"
#include "bs_tracing.h"
#include "bs_oswrap.h"
#include "bs_utils.h"
#include "bs_rand_main.h"
#include "bs_pc_2G4.h"
#include "bs_pc_2G4_utils.h"
#include "modem_if.h"
#include "p2G4_pending_tx_rx_list.h"
#include "modem_BLE_simple.h"
#include "modem_BLE_simple_args.h"
#include <math.h>

#define FreqRes 200e3

//Rx filters "_power_ gain" (gain in natural units ^2) (the gain for the desired modulation should be 0dB)
static const double channel_filter_BLE[41] = {
    1.0000e-06,   1.2589e-06,   1.5849e-06,   1.9953e-06,   2.5119e-06,
    3.1623e-06,   5.0119e-06,   1.0000e-05,   3.1623e-05,   1.0000e-04,
    3.1623e-04,   1.0000e-03,   3.1623e-03,   1.0000e-02,   3.9811e-02,
    1.1398e-01,   2.6304e-01,   6.7212e-01,   7.9446e-01,   9.8531e-01,
    1.1791e+00,   9.8531e-01,   7.9446e-01,   6.7212e-01,   2.6304e-01,
    1.1398e-01,   3.9811e-02,   1.0000e-02,   3.1623e-03,   1.0000e-03,
    3.1623e-04,   1.0000e-04,   3.1623e-05,   1.0000e-05,   5.0119e-06,
    3.1623e-06,   2.5119e-06,   1.9953e-06,   1.5849e-06,   1.2589e-06,
    1.0000e-06
};
static const uint channel_filter_BW_BLE = 20; //How many samples from the center frequency

static const double channel_filter_Prop2[81] = {
    1.0000e-06,   1.5849e-06,   2.5119e-06,   3.9811e-06,   6.3096e-06,
    7.9433e-06,   1.0000e-05,   1.2589e-05,   1.5849e-05,   1.9953e-05,
    2.5119e-05,   3.1623e-05,   3.9811e-05,   5.0119e-05,   6.3096e-05,
    7.9433e-05,   1.0000e-04,   1.2589e-04,   1.5849e-04,   1.9953e-04,
    2.5119e-04,   3.1623e-04,   3.9811e-04,   5.0119e-04,   6.3096e-04,
    1.0000e-03,   1.5849e-03,   3.1623e-03,   6.3096e-03,   3.1565e-02,
    5.2087e-02,   6.4973e-02,   1.3748e-01,   2.0369e-01,   4.3440e-01,
    8.5992e-01,   1.0097e+00,   1.0097e+00,   1.0097e+00,   1.0097e+00,
    1.0097e+00,   1.0097e+00,   1.0097e+00,   1.0097e+00,   1.0097e+00,
    8.5992e-01,   4.3440e-01,   2.0369e-01,   1.3748e-01,   6.4973e-02,
    5.2087e-02,   3.1565e-02,   6.3096e-03,   3.1623e-03,   1.5849e-03,
    1.0000e-03,   6.3096e-04,   5.0119e-04,   3.9811e-04,   3.1623e-04,
    2.5119e-04,   1.9953e-04,   1.5849e-04,   1.2589e-04,   1.0000e-04,
    7.9433e-05,   6.3096e-05,   5.0119e-05,   3.9811e-05,   3.1623e-05,
    2.5119e-05,   1.9953e-05,   1.5849e-05,   1.2589e-05,   1.0000e-05,
    7.9433e-06,   6.3096e-06,   3.9811e-06,   2.5119e-06,   1.5849e-06,
    1.0000e-06
};
static const uint channel_filter_BW_Prop2 = 40; //How many samples from the center frequency

//_Power_ spectral density of each modulation (in natural units, power (sum must be 1.0))
static const double TxSpectrum_BLE[13] = {
    4.1980e-05,   2.8516e-04,   4.8379e-04,   1.2872e-02,   9.1159e-02,
    2.4433e-01,   3.0165e-01,   2.4433e-01,   9.1159e-02,   1.2872e-02,
    4.8379e-04,   2.8516e-04,   4.1980e-05};
static const uint TxSpecBW_BLE = 6;

static const double TxSpectrum_Prop2[25] =
{ 1.4055e-05,   5.6012e-05,   1.5134e-04,   2.1890e-04,   1.2972e-04,
    3.2762e-04,   3.3251e-03,   1.4459e-02,   4.1335e-02,   8.0105e-02,
    1.1656e-01,   1.5506e-01,   1.7652e-01,   1.5506e-01,   1.1656e-01,
    8.0105e-02,   4.1335e-02,   1.4459e-02,   3.3251e-03,   3.2762e-04,
    1.2972e-04,   2.1890e-04,   1.5134e-04,   5.6012e-05,   1.4055e-05};
static const uint TxSpecBW_Prop2 = 12;

static const double TxSpectrum_WLAN[123] = {
    1.511874e-006 ,3.797656e-006 ,9.539280e-006 ,1.511874e-005 ,2.396159e-005 ,
    3.797656e-005 ,6.753293e-005 ,9.539280e-005 ,1.347459e-004 ,1.903337e-004 ,
    2.688534e-004 ,3.797656e-004 ,4.565786e-004 ,3.016585e-004 ,3.585218e-004 ,
    5.617144e-004 ,7.071566e-004 ,8.501892e-004 ,7.577319e-004 ,1.200924e-003 ,
    1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,
    1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,
    1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,
    1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,
    1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,
    1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,
    1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,
    1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,
    1.200924e-002 ,2.396159e-003 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,
    1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,
    1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,
    1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,
    1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,
    1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,
    1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,
    1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,
    1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-003 ,7.577319e-004 ,
    8.501892e-004 ,7.071566e-004 ,5.617144e-004 ,3.585218e-004 ,3.016585e-004 ,
    4.565786e-004 ,3.797656e-004 ,2.688534e-004 ,1.903337e-004 ,1.347459e-004 ,
    9.539280e-005 ,6.753293e-005 ,3.797656e-005 ,2.396159e-005 ,1.511874e-005 ,
    9.539280e-006 ,3.797656e-006 ,1.511874e-006
};
static const uint TxSpecBW_WLAN = 61;

static const double TxSpectrum_CW[1]  = {1};
static const uint TxSpecBW_CW = 0;
static const double TxSpectrum_WN1MHz[5] = {
    1.0/5 , 1.0/5.0, 1.0/5.0, 1.0/5.0, 1.0/5
};
static const uint TxSpecBW_WN1MHz = 2;
static const double TxSpectrum_WN2MHz[11] = {
    1.0/20.0, 1.0/10.0, 1.0/10.0, 1.0/10.0, 1.0/10.0,
    1.0/10.0, 1.0/10.0, 1.0/10.0, 1.0/10.0, 1.0/10.0,
    1.0/20.0
};
static const uint TxSpecBW_WN2MHz = 5;
static const double TxSpectrum_WN4MHz[21] = {
    1.0/40.0, 1.0/20.0, 1.0/20.0, 1.0/20.0, 1.0/20.0,
    1.0/20.0, 1.0/20.0, 1.0/20.0, 1.0/20.0, 1.0/20.0,
    1.0/20.0, 1.0/20.0, 1.0/20.0, 1.0/20.0, 1.0/20.0,
    1.0/20.0, 1.0/20.0, 1.0/20.0, 1.0/20.0, 1.0/20.0,
    1.0/40.0
};
static const uint TxSpecBW_WN4MHz = 10;

static const double TxSpectrum_WN8MHz[41] = {
    1.0/80.0, 1.0/40.0, 1.0/40.0, 1.0/40.0, 1.0/40.0, 1.0/40.0, 1.0/40.0, 1.0/40.0, 1.0/40.0, 1.0/40.0,
    1.0/40.0, 1.0/40.0, 1.0/40.0, 1.0/40.0, 1.0/40.0, 1.0/40.0, 1.0/40.0, 1.0/40.0, 1.0/40.0, 1.0/40.0,
    1.0/40.0, 1.0/40.0, 1.0/40.0, 1.0/40.0, 1.0/40.0, 1.0/40.0, 1.0/40.0, 1.0/40.0, 1.0/40.0, 1.0/40.0,
    1.0/40.0, 1.0/40.0, 1.0/40.0, 1.0/40.0, 1.0/40.0, 1.0/40.0, 1.0/40.0, 1.0/40.0, 1.0/40.0, 1.0/40.0,
    1.0/80.0
};

static const uint   TxSpecBW_WN8MHz = 20;

static const double TxSpectrum_WN16MHz[81] = {
    1.0/160.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0,
    1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0,
    1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0,
    1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0,
    1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0,
    1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0,
    1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0,
    1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0, 1.0/80.0,
    1.0/160.0
};
static const uint   TxSpecBW_WN16MHz = 40;

static const double TxSpectrum_WN20MHz[101] = {
    1.0/200.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, //1
    1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, //2
    1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, //3
    1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, //4
    1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, //5
    1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, //6
    1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, //7
    1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, //8
    1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, //9
    1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, 1.0/100.0, //10
    1.0/200.0
};

static const uint   TxSpecBW_WN20MHz = 50;

static const double TxSpectrum_WN40MHz[201] = {
    1.0/400.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, //1
    1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, //2
    1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, //3
    1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, //4
    1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, //5
    1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, //6
    1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, //7
    1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, //8
    1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, //9
    1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, //10
    1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, //11
    1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, //12
    1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, //13
    1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, //14
    1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, //15
    1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, //16
    1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, //17
    1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, //18
    1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, //19
    1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, 1.0/200.0, //20
    1.0/400.0
};
static const uint   TxSpecBW_WN40MHz = 100;

static const double TxSpectrum_WN80MHz[401] = {
    1.0/800.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //1
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //2
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //3
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //4
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //5
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //6
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //7
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //8
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //9
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //10
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //11
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //12
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //13
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //14
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //15
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //16
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //17
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //18
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //19
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //20
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //21
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //22
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //23
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //24
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //25
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //26
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //27
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //28
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //29
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //30
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //31
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //32
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //33
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //34
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //35
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //36
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //37
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //38
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //39
    1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, 1.0/400.0, //40
    1.0/800.0
};
static const uint TxSpecBW_WN80MHz = 200;

/**
 * Return the Rx filter equivalent noise BW for a given Rx modulation configuration
 *
 * Note that these values do not need to necessarily match the real Rx filter noise BW.
 * As they really are just a factor in the thermal noise calculation they may
 * just include any other effect which would affect the resulting thermal noise component
 * in the demodulator input for that given modulation.
 */
static double noiseBW_from_modulation(uint modulation_type) {
  switch (modulation_type) {
    case P2G4_MOD_BLE:
    case P2G4_MOD_BLE_CODED:
    case P2G4_MOD_154_250K_DSS: //Provisionally let's treat 15.4 like 1Mbps BLE
      return 1.1e6;
      break;
    case P2G4_MOD_BLE2M:
    case P2G4_MOD_PROP2M:
      return 2.5e6;
      break;
    default:
      bs_trace_error_line("modem BLE Simple does not support receiving "
                          "configured with this modulation type (%i)\n",
                          modulation_type);
      break;
  }
  return 0;
}

static void RxFilter_from_modulation(uint modulation_type, const double ** filter_attenuation,
                                     uint *filter_half_BW) {

  switch (modulation_type) {
    case P2G4_MOD_BLE:
    case P2G4_MOD_BLE_CODED:
    case P2G4_MOD_154_250K_DSS: //Provisionally let's treat 15.4 like 1Mbps BLE
      *filter_attenuation = channel_filter_BLE;
      *filter_half_BW     = channel_filter_BW_BLE;
      break;
    case P2G4_MOD_BLE2M:
    case P2G4_MOD_PROP2M:
      *filter_attenuation = channel_filter_Prop2;
      *filter_half_BW     = channel_filter_BW_Prop2;
      break;
    default:
      bs_trace_error_line("modem BLE Simple does not support receiving "
                          "configured with this modulation type (%i)\n",
                          modulation_type);
      break;
  }
}

static void spectrum_from_modulation(uint modulation_type, const double **tx_spectrum,
                                     uint *tx_spec_halfBW) {

  switch (modulation_type) {
    case P2G4_MOD_BLE :
    case P2G4_MOD_BLE_CODED:
    case P2G4_MOD_154_250K_DSS: //Provisionally let's treat 15.4 like 1Mbps BLE
      *tx_spectrum    = TxSpectrum_BLE;
      *tx_spec_halfBW = TxSpecBW_BLE;
      break;
    case P2G4_MOD_BLE2M:
    case P2G4_MOD_PROP2M:
      *tx_spectrum    = TxSpectrum_Prop2;
      *tx_spec_halfBW = TxSpecBW_Prop2;
      break;
    case P2G4_MOD_PROP4M :
      *tx_spectrum    = TxSpectrum_WN4MHz;
      *tx_spec_halfBW = TxSpecBW_WN4MHz;
      break;
    case P2G4_MOD_CWINTER :
      *tx_spectrum    = TxSpectrum_CW;
      *tx_spec_halfBW = TxSpecBW_CW;
      break;
    case P2G4_MOD_WHITENOISE1MHz :
      *tx_spectrum    = TxSpectrum_WN1MHz;
      *tx_spec_halfBW = TxSpecBW_WN1MHz;
      break;
    case P2G4_MOD_WHITENOISE2MHz :
      *tx_spectrum    = TxSpectrum_WN2MHz;
      *tx_spec_halfBW = TxSpecBW_WN2MHz;
      break;
    case P2G4_MOD_WHITENOISE4MHz :
      *tx_spectrum    = TxSpectrum_WN4MHz;
      *tx_spec_halfBW = TxSpecBW_WN4MHz;
      break;
    case P2G4_MOD_WHITENOISE8MHz :
      *tx_spectrum    = TxSpectrum_WN8MHz;
      *tx_spec_halfBW = TxSpecBW_WN8MHz;
      break;
    case P2G4_MOD_WHITENOISE16MHz :
      *tx_spectrum    = TxSpectrum_WN16MHz;
      *tx_spec_halfBW = TxSpecBW_WN16MHz;
      break;
    case P2G4_MOD_WHITENOISE20MHz :
      *tx_spectrum    = TxSpectrum_WN20MHz;
      *tx_spec_halfBW = TxSpecBW_WN20MHz;
      break;
    case P2G4_MOD_WHITENOISE40MHz :
      *tx_spectrum    = TxSpectrum_WN40MHz;
      *tx_spec_halfBW = TxSpecBW_WN40MHz;
      break;
    case P2G4_MOD_WHITENOISE80MHz :
      *tx_spectrum    = TxSpectrum_WN80MHz;
      *tx_spec_halfBW = TxSpecBW_WN80MHz;
      break;
    case P2G4_MOD_WLANINTER :
      *tx_spectrum    = TxSpectrum_WLAN;
      *tx_spec_halfBW = TxSpecBW_WLAN;
      break;
    default:
      bs_trace_warning_line("modem BLE Simple does not support seeing this "
                            "modulation type (%i) in the air, it will be "
                            "treated as 1MHz white noise\n",modulation_type);
      *tx_spectrum    = TxSpectrum_WN1MHz;
      *tx_spec_halfBW = TxSpecBW_WN1MHz;
      break;
  }
}

/**
 * Q function:
 *   Q(x) = P(X > x) , where X = N(0,1)
 *   That is, Q(x) is the probability of a N(0,1) having a value > x
 *   where N(0,1) is a Normal distribution with average 0, and standard deviation 1
 */
static double Q_function(double x) {
  return 1.0/2.0*erfc(x/sqrt(2.0));
}

/************
 * Modem IF *
 ************/

/**
 * Initialize the modem internal status
 *
 * The input parameters are:
 *   dev_nbr      : the device number this modem library corresponds to
 *                  (note that this library may be reused in between several devices)
 *   nbr_devices  : the total number of devices in the simulation (gives the length of vectors later)
 *   char *argv[] : a set of arguments passed to the modem library (same convention as POSIX main() command line arguments)
 *   int argc     : the number of arguments passed to the modem library
 *
 * It is up to the modem to define which arguments it can or shall receive to configure it
 * The only argument all modems shall understand is :
 *    [-h] [--h] [--help] [-?]  The modem shall print a list of arguments it supports with a descriptive message for each
 *
 * This function shall return a pointer to this modems status structure
 * This same pointer will be passed back to consecutive function calls of the library (void *this)
 *
 * If the library does not require to keep any status it can just return NULL
 */
void* modem_init(int argc, char *argv[], uint d, uint n_devs) {
  m_simple_status_t* mo_st;

  mo_st = (m_simple_status_t *)bs_calloc(1,sizeof(m_simple_status_t));

  mo_st->nbr_devices = n_devs;

  modem_simple_argparse(argc, argv, d, &mo_st->args);

  mo_st->device_RSSI_offsetdB = bs_random_Gaus()*mo_st->args.RSSI_offsetdB_std;

  return (void*)mo_st;
}

/**
 * Calculate the SNR of the desired input signal at the digital modem input/analog output
 * including all modem analog impairments
 *
 * inputs:
 *  this               : Pointer to this modem object
 *  rx_radioparams     : Radio parameters/configuration of this receiver for this Rx/RSSI measurement
 *  rx_powers          : For each possible transmitter ([0..n_devices-1]) what is their power level at this device antenna connector in dBm
 *  txl_c              : For each possible transmitter what are their Tx parameters
 *  desired_tx_nbr     : which of the transmitters is the one we are trying to receive
 *
 * outputs:
 *  OutputSNR               : SNR in the analog output / digital input of this modem (dB)
 *  Output_RSSI_power_level : RSSI level (analog, dBm) sent to the modem digital
 */
void modem_analog_rx(void *this, p2G4_radioparams_t *rx_radioparams, double *OutputSNR,
                     double *Output_RSSI_power_level, double *rx_powers, tx_l_c_t *txl_c,
                     uint desired_tx_nbr) {
  m_simple_status_t *mo_st = (m_simple_status_t *)this;
  uint i;

  double ThermalNoisemW = 0;
  double TotalInterfmW  = 0;
  double TotalPhaseReciprocalMixmW = 0; //If we were more precise, we would also calculate here the phase noise mixing
  double TotalIntermmW  = 0; //Intermodulation noise. This can be very significant if there is any blocker present, but we don't include it in this model

  { //Thermal noise calculation
    double NoiseFigure = mo_st->args.NFigure_ana; //dB
    double EquivalentBW = noiseBW_from_modulation(rx_radioparams->modulation);
    ThermalNoisemW = EquivalentBW * pow(10, (-174.0 + NoiseFigure )/10);
  }

  { //Interference calculation (including adjacent channel)
    const double *RxFilterAttenuation;
    uint RxFilterHalfBW;

    RxFilter_from_modulation(rx_radioparams->modulation, &RxFilterAttenuation, &RxFilterHalfBW);
    //The "Rx filter" shall include all effects which would increase
    //the noise/interference power at a given frequency offset

    int CenterR = p2G4_freq_to_d(rx_radioparams->center_freq) / (FreqRes/1e6); //Center frequency (where 0 = 2400MHz)
    int StartR  = CenterR - RxFilterHalfBW;
    int EndR    = CenterR + RxFilterHalfBW;

    for ( i = 0; i < mo_st->nbr_devices ; i++ ) {
      if ( txl_c->used[i] && ( i != desired_tx_nbr ) ){ //For each active transmitter
        //Calculate how much power comes thru the Rx channel/IF filtering

        const double *TxSpectrum;
        uint TxSpecHalfBW;

        spectrum_from_modulation(txl_c->tx_list[i].tx_s.radio_params.modulation, &TxSpectrum, &TxSpecHalfBW);

        int CenterI = p2G4_freq_to_d(txl_c->tx_list[i].tx_s.radio_params.center_freq) / (FreqRes/1e6); //Center frequency (where 0 = 2400MHz)
        int StartI  = CenterI - TxSpecHalfBW;
        int EndI    = CenterI + TxSpecHalfBW;

        int StartOverlap = BS_MAX(StartR,StartI);
        int EndOverlap   = BS_MIN(EndR, EndI);

        double AccRatio = 0; //Calculate the overlap of the Rx filter with the tx modulation shape
        for (int Freq = StartOverlap; Freq <= EndOverlap ; Freq++) {
           int IndexR = (Freq - StartR);
           int IndexI = (Freq - StartI);
           AccRatio = AccRatio + TxSpectrum[IndexI] * RxFilterAttenuation[IndexR];
        }

        TotalInterfmW = TotalInterfmW + AccRatio * pow(10.0, rx_powers[i]/10.0);

      }
    }
  } //End of Interference calc.

  { //Adding it all together
    double DesiredPowerdBm;
    double TotalNoisemW = ThermalNoisemW + TotalInterfmW + TotalPhaseReciprocalMixmW + TotalIntermmW;
    double TotalSignalEtNoisemW;

    if ( desired_tx_nbr >= mo_st->nbr_devices ){
      DesiredPowerdBm = -1000; //-1000dB == nothing
    } else {
      DesiredPowerdBm = rx_powers[desired_tx_nbr];
    }

    TotalSignalEtNoisemW = TotalNoisemW + pow( 10.0, DesiredPowerdBm/10.0 );

    *OutputSNR = DesiredPowerdBm - 10.0*log10( TotalNoisemW );
    *Output_RSSI_power_level = 10*log10(TotalSignalEtNoisemW);
  }
}

/**
 * Return the bit error probability ([0.. RAND_PROB_1]) for a given SNR
 *
 * inputs:
 *  this           : Pointer to this modem object
 *  rx_radioparams : Radio parameters/configuration of this receiver for this Rx/RSSI measurement
 *  SNR            : SNR level at the analog output as calculated by modem_analog_rx()
 *
 *
 * Note: For coded modulations (BLE CodedPhy):
 *   * The assumption is that this will be called for each "uncoded" bit (i.e. at a rate of 125
 *     or 500kbps), and not for the underlying channel coded bits (i.e. at 1Mbps). That is, this
 *     model accounts for the gain of the convolutional coding."
 *   * Note that by now, for CodedPhy this will just provide an offset version of the 1Mbps
 *     performance, to account roughly for the coding gain. More precise performance accounting
 *     for a typical Viterbi decoder is pending.
 */
uint32_t modem_digital_perf_ber(void *this, p2G4_modemdigparams_t *rx_modem_params, double SNR) {
  m_simple_status_t *mo_st = (m_simple_status_t *)this;

  double coding_gain = 0;

  if (rx_modem_params->modulation == P2G4_MOD_BLE_CODED) {
    //By now let's just offset the curve by 4 and 9dB respectively
    //It is not too accurate, but much better than nothing
    if (rx_modem_params->coding_rate == 2) { //S=2
      coding_gain = 4;
    } else { //S=8
      coding_gain = 9;
    }
  }

  //Just the theoretical BER vs Eb/No | SNR
  // BER = Q(sqrt(Eb/No)) = 1/2*erfc(sqrt(Eb/(2*No)))

  /*SNR = SNR - NFigure;
    N_u = 1./10.^((SNR)/10);
    N_u_o = N_u + 10.^(NoiseFloor/10);
    SNR_2 = 10*log10(1./N_u_o); */

  SNR = SNR - mo_st->args.NFigure_extra + coding_gain;
  double N_u = 1.0/pow(10.0,SNR/10.0); //equivalent noise level level relative to the signal power
  double N_u_o = N_u + pow(10.0, mo_st->args.NFloor_dig/10); //add the noise equivalent to the noise floor (relative to the singal power also)

  double SNR_nu = 1.0/N_u_o; //back to SNR (in natural units)
  //SNR = 10.0*log10(1.0/N_u_o);

  double BER = Q_function(sqrt(SNR_nu)); //BER from [0..1.0]
  //BER = Q_function(pow(10.0,SNR/20));

  return BER*RAND_PROB_1 + 0.5; //scaled to 32bits and rounded to nearest int.
}

/**
 * Return the probability of the packet sync'ing ([0.. RAND_PROB_1]) for a given SNR and
 * transmission parameters
 *
 * (note that this should ONLY include excess packet error probability,
 * as over the sync word and address normal bit errors will also be calculated)
 *
 * inputs:
 *  this           : Pointer to this modem object
 *  rx_radioparams : Radio parameters/configuration of this receiver for this Rx/RSSI measurement
 *  SNR            : SNR level at the analog output as calculated by modem_analog_rx()
 *  tx_s           : Parameters of the transmission we are receiving (in case the sync. probability depends on any of them)
 */
uint32_t modem_digital_perf_sync(void *this, p2G4_modemdigparams_t *rx_modem_params,
                                 double SNR, p2G4_txv2_t* tx_s) {
  m_simple_status_t *mo_st = (m_simple_status_t *)this;
  mo_simple_args_t *args = &mo_st->args;

  return args->Sync_prob;
}

/**
 * Return the digital RSSI value the modem would have produced for this given
 * RSSI_power_level in the digital input
 *
 * inputs:
 *  this             : Pointer to this modem object
 *  rx_radioparams   : Radio parameters/configuration of this receiver for this Rx/RSSI measurement
 *  RSSI_power_level : Analog power level as measured by modem_analog_rx()
 *
 * outputs:
 *  RSSI  : RSSI "digital" value returned by this modem, following the p2G4_rssi_power_t format (16.16 signed value)
 */
void modem_digital_RSSI(void *this, p2G4_radioparams_t *rx_radioparams,
                        double RSSI_power_level, p2G4_rssi_power_t* RSSI) {

  m_simple_status_t *mo_st = (m_simple_status_t *)this;

  RSSI_power_level += mo_st->device_RSSI_offsetdB; //average measurement offset of this device relative to a perfect device
  RSSI_power_level += bs_random_Gaus() * mo_st->args.RSSI_meas_noisedB_std; //measurement noise

  *RSSI = p2G4_RSSI_value_from_dBm(RSSI_power_level);
}

/**
 * Clean up: Free the memory the modem may have allocated
 * close any file descriptors etc.
 * (the simulation has ended)
 */
void modem_delete(void *this) {
  if (this != NULL) {
    free(this);
  }
}
