# Copyright Oticon A/S 2018
# SPDX-License-Identifier: Apache-2.0

function [Power] = calc_interf( List_Inter, ModulationRx )

FilterRx = [0.5, 1, 1, 1, 0.5];
