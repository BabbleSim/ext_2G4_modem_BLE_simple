# Copyright Oticon A/S 2018
# SPDX-License-Identifier: Apache-2.0

function [SNR, RSSI] = analog_model( ListInter, Desired, ModulationRx, CenterFreq )
PlotF = 0;

%%Thermal noise calculation
NoiseFigure = 4; %dB
                 % Note that in reality the noise figure will increase as the gain decreases, but anyway the thermal noise wont be significant once we have higher levels

%ThermalNoise =  -174dBm/Hz * ReceiverBW * NoiseFigure
ThermalNoisemW = 10^( (-174 + NoiseFigure)/10 ) * ReceiverNoiseBW(ModulationRx);

%end of thermal noise
%%Interference calculation
[FilterRx, HalfBW, SpectrumSamplingBW] = RxFilterShape( ModulationRx );

StartR = (CenterFreq - 2400e6)/SpectrumSamplingBW - HalfBW;
EndR   = (CenterFreq - 2400e6)/SpectrumSamplingBW + HalfBW;

if PlotF == 1,
  figure(1);
  hold off;
  plot( (StartR:EndR), 10*log10(FilterRx) , 'x','DisplayName','Rx Filter');
  hold all;
  plot( (CenterFreq-2400e6)/SpectrumSamplingBW, Desired.RxPower , '^','DisplayName','Desired Signal');
end

InterfPowermW = 0;

for i = 1:numel(ListInter),
  [ListInter{i}.Spectrum , ListInter{i}.HalfBW, ~] = ModSpectrum( ListInter{i}.Modulation );
 
  StartI = (ListInter{i}.CenterFreq - 2400e6)/SpectrumSamplingBW - ListInter{i}.HalfBW;
  EndI   = (ListInter{i}.CenterFreq - 2400e6)/SpectrumSamplingBW + ListInter{i}.HalfBW;
  
  StartOverlap = max(StartR, StartI);
  EndOverlap = min(EndR, EndI);
  
  if PlotF == 1,
    plot( (StartI:EndI), 10*log10(ListInter{i}.Spectrum.* 10^(ListInter{i}.RxPower/10)) ,'o','DisplayName',['Interf. ' num2str(i)]);
  end
   
  AccRatio = 0;
  for Freq = StartOverlap:EndOverlap,
    IndexR = (Freq - StartR) + 1; %matlab indexing
    IndexI = (Freq - StartI) + 1; %matlab indexing
    AccRatio = AccRatio + ListInter{i}.Spectrum(IndexI) * FilterRx(IndexR);
  end
  InterfPowermW = InterfPowermW + AccRatio * 10^(ListInter{i}.RxPower/10);
end
if PlotF == 1,
  legend('Location','Best');
end
%End of interference calculation
TotalNoisemW = InterfPowermW + ThermalNoisemW;

SNR = Desired.RxPower - 10*log10(TotalNoisemW);
RSSI = 10*log10( 10^(Desired.RxPower/10) + TotalNoisemW );
