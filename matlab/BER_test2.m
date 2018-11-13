# Copyright Oticon A/S 2018
# SPDX-License-Identifier: Apache-2.0

function [] = BER_test2() 
%script to test the BER calculation of the C model (second part of test_ber.c)

  load ../src/test/BER.txt
  level = BER(:,1);
  SNR = BER(:,2);
  BER = BER(:,3);
  
  BER_2 = zeros(size(level));
  
  ModulationRx = 'modulation_BLE';
  CenterFreq = 2450e6;
  clear ListInter
  ListInter = {};
  
  for level_i = level',
    Desired.RxPower = level_i;
    [SNR, ~] = analog_model( ListInter, Desired, ModulationRx, CenterFreq );
    BER_2(level_i==level) = digital_ber(SNR);
  end
  figure(1); clf;
  semilogy(level,BER,'r'); hold on;
  semilogy(level,BER_2,'b'); grid on;
  ylim([10^-8 1]); xlabel('dBm'); ylabel('BER');
  xlim([-105 -90]);
end

