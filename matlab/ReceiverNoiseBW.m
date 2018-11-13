# Copyright Oticon A/S 2018
# SPDX-License-Identifier: Apache-2.0

function [ BW ] = ReceiverNoiseBW(ModulationRx)

if ( strcmp(ModulationRx, 'modulation_BLE')== 1 ),
  BW = 1.1e6;
elseif ( strcmp(ModulationRx, 'modulation_Prop2M')== 1 ),
  BW = 2.5e6;
else
  error(' I cant handle that modulation for the Rx filter');
end
