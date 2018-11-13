# Copyright Oticon A/S 2018
# SPDX-License-Identifier: Apache-2.0

function [Spectrum, HalfBW, SpectrumSamplingBW] = ModSpectrum( Modulation )

SpectrumSamplingBW = 200e3;

%Power spectral density of each modulation in natural units 

%They are normalized to have a power of 1 over the whole spectrum

if ( strcmp(Modulation, 'modulation_BLE')== 1 ),
  Spectrum = [4.1980e-05   2.8516e-04   4.8379e-04   1.2872e-02   9.1159e-02   2.4433e-01   3.0165e-01   2.4433e-01   9.1159e-02   1.2872e-02   4.8379e-04 ...
              2.8516e-04   4.1980e-05];
  HalfBW = 6;
elseif ( strcmp(Modulation, 'modulation_Prop2M')== 1 ),
  HalfBW = 12;
   Spectrum = [1.4055e-05   5.6012e-05   1.5134e-04   2.1890e-04   1.2972e-04   3.2762e-04   3.3251e-03   1.4459e-02   4.1335e-02   8.0105e-02   1.1656e-01 ...
   1.5506e-01   1.7652e-01   1.5506e-01   1.1656e-01   8.0105e-02   4.1335e-02   1.4459e-02   3.3251e-03   3.2762e-04   1.2972e-04   2.1890e-04 ...
   1.5134e-04   5.6012e-05   1.4055e-05];
elseif ( strcmp(Modulation, 'modulation_CWInter')== 1 ),
  Spectrum = [1];
  HalfBW = 0;
elseif ( strcmp(Modulation, 'modulation_WhiteNoise1MHz')== 1 ),
  Spectrum = [1.0/5 , 1.0/5.0, 1.0/5.0, 1.0/5.0, 1.0/5];
  HalfBW = 2;
elseif ( strcmp(Modulation, 'modulation_WhiteNoise2MHz')== 1 ),
  Spectrum = [1/20, 1/10*ones(1,9), 1/20];
  HalfBW = 5;
elseif ( strcmp(Modulation, 'modulation_WhiteNoise4MHz')== 1 ) || ( strcmp(Modulation, 'modulation_Prop4M')== 1 ),
  Spectrum = [1/40, 1/20*ones(1,19), 1/40];
  HalfBW = 10;
elseif ( strcmp(Modulation, 'modulation_WhiteNoise8MHz')== 1 ),
  Spectrum = [1/80, 1/40*ones(1,39), 1/80];
  HalfBW = 20;
elseif ( strcmp(Modulation, 'modulation_WhiteNoise16MHz')== 1 ),
  Spectrum = [1/160, 1/80*ones(1,79), 1/160];
  HalfBW = 40;
elseif ( strcmp(Modulation, 'modulation_WhiteNoise20MHz')== 1 ),
  Spectrum = [1/200, 1/100*ones(1,99), 1/200];
  HalfBW = 50;
elseif ( strcmp(Modulation, 'modulation_WhiteNoise40MHz')== 1 ),
  Spectrum = [1/400, 1/200*ones(1,199), 1/400];
  HalfBW = 100;
elseif ( strcmp(Modulation, 'modulation_WhiteNoise80MHz')== 1 ),
  Spectrum = [1/800, 1/400*ones(1,399), 1/800];
  HalfBW = 200;
elseif ( strcmp(Modulation, 'modulation_WLANInter')== 1 ),
  Spectrum = [1.511874e-006 ,3.797656e-006 ,9.539280e-006 ,1.511874e-005 ,2.396159e-005 ,3.797656e-005 ,6.753293e-005 ,9.539280e-005 ,1.347459e-004 ,1.903337e-004 ,2.688534e-004 ,3.797656e-004 ,4.565786e-004 ,3.016585e-004 ,3.585218e-004 ,5.617144e-004 ,7.071566e-004 ,8.501892e-004 ,7.577319e-004 ,1.200924e-003 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,2.396159e-003 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-002 ,1.200924e-003 ,7.577319e-004 ,8.501892e-004 ,7.071566e-004 ,5.617144e-004 ,3.585218e-004 ,3.016585e-004 ,4.565786e-004 ,3.797656e-004 ,2.688534e-004 ,1.903337e-004 ,1.347459e-004 ,9.539280e-005 ,6.753293e-005 ,3.797656e-005 ,2.396159e-005 ,1.511874e-005 ,9.539280e-006 ,3.797656e-006 ,1.511874e-006 ];
  HalfBW = 61;
else
  error(' I cant handle that modulation for the Rx filter');
end
