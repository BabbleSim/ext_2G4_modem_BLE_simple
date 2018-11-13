# Copyright Oticon A/S 2018
# SPDX-License-Identifier: Apache-2.0

function BER = digital_ber(SNR)

  NFigure = 2; %in dB, extra noise figure of the digital demodulation path
  NoiseFloor = -35; %in dB, maximum SNR (equivalent to a quantization noise)
  
  SNR_i = SNRLossv2(SNR,NFigure,NoiseFloor);

  tmp = 10.^(SNR_i/20); %/20 instead of 10 to get the sqrt(); 
  BER = alpi_q_func(tmp); 
end

function SNR_2 = SNRLossv2(SNR,NFigure,NoiseFloor)
  % SNR degradation produced internally by the receiver
  %
  % the input SNR (S/N) is transformed into
  % S/(NFigure*N + S*Nfloor) (in normal units)

  SNR = SNR - NFigure;

  N_u = 1./10.^((SNR)/10);

  N_u_o = N_u + 10.^(NoiseFloor/10) ;

  SNR_2 = 10*log10(1./N_u_o) ;
end

function [value] = alpi_q_func(x)
  %function [value] = alpi_q_func(x)
  % Home made version of the matlab qfunc()
  % the normal qfunc() requires the communications toolbox...
  % http://www.mathworks.se/help/comm/ref/qfunc.html

  value = 1/2*erfc(x/sqrt(2));
end