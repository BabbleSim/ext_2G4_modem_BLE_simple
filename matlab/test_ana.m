# Copyright Oticon A/S 2018
# SPDX-License-Identifier: Apache-2.0

ModulationRx = 'modulation_Prop2M';
CenterFreq = 2450e6;

Desired.RxPower = -60; %dBm

clear ListInter
ListInter = {};
ListInter{1}.Modulation = 'modulation_BLE';
ListInter{1}.CenterFreq = 2448e6;
ListInter{1}.RxPower    = -70;
 
ListInter{2}.Modulation = 'modulation_WhiteNoise1MHz';
ListInter{2}.CenterFreq = 2446e6;
ListInter{2}.RxPower    = -60;
  
ListInter{3}.Modulation = 'modulation_CWInter';
ListInter{3}.CenterFreq = 2446e6;
ListInter{3}.RxPower    = -50;
  
ListInter{4}.Modulation = 'modulation_WhiteNoise4MHz';
ListInter{4}.CenterFreq = 2449e6;
ListInter{4}.RxPower    = -80;
 
ListInter{5}.Modulation = 'modulation_WhiteNoise2MHz';
ListInter{5}.CenterFreq = 2451e6;
ListInter{5}.RxPower    = -80;

ListInter{6}.Modulation = 'modulation_Prop4M';
ListInter{6}.CenterFreq = 2448e6;
ListInter{6}.RxPower    = -70;

ListInter{7}.Modulation = 'modulation_Prop2M';
ListInter{7}.CenterFreq = 2448e6;
ListInter{7}.RxPower    = -60;


[SNR, RSSI] = analog_model( ListInter, Desired, ModulationRx, CenterFreq )
