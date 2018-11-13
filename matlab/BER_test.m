# Copyright Oticon A/S 2018
# SPDX-License-Identifier: Apache-2.0

function [] = BER_test() 
%script to test the BER calculation of the C model (test_ber.c)

  load ../src/test/BER.txt
  SNR = BER(:,1);
  BER = BER(:,2);
  
  BER_2 = digital_ber(SNR);
  figure(1); clf;
  semilogy(SNR,BER,'r'); hold on;
  semilogy(SNR,BER_2,'b'); grid on;
  ylim([10^-8 1]);
end
