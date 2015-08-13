#!/bin/bash

# Copyright 2015 Ankur Sinha 
# Author: Ankur Sinha <sanjay DOT ankur AT gmail DOT com> 
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# File : 
#

SOURCEDIR=/home/asinha/Documents/02_Code/00_repos/00_mine/herts-research-repo/src/
SCRIPTDIR="$SOURCEDIR""postprocess/scripts/"

for i in "0" "0.01" "0.015" "0.02" "0.025" "0.03" "0.035" "0.036" "0.037" "0.038" "0.039" "0.040" "0.041" "0.042" "0.043" "0.044"  "0.045" "0.046" "0.047" "0.048" "0.049" "0.050" "0.051" "0.052" "0.053" "0.054" "0.055" "0.056" "0.057" "0.058" "0.059" "0.06" "0.065" "0.07"
do
    echo "k_ex is $i"
    mkdir "k_ex_$i"
    pushd "k_ex_$i"
        cp "$SOURCEDIR"/brian2/adex.py .
        sed -i "s|k_ex = .*$|k_ex = $i|" adex.py
        echo "Running python program with k_ex $i."
        python adex.py
    popd
done

for i in k_ex*
do
    cat "$i"/firing-rates-e.txt >> firing-rates-e.txt
    cat "$i"/firing-rates-i.txt >> firing-rates-i.txt
done
gnuplot "$SCRIPTDIR""f-w_ee-graphs.plt"
