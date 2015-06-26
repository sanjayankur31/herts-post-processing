#!/bin/bash

# Copyright 2010 Ankur Sinha 
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
# File : multiple_e_fi.sh - generate an FI curve for AdEx a,b=0 with different e_reset values.
#

SOURCEDIR=/home/asinha/Documents/02_Code/00_repos/00_mine/herts-research-repo/src/

for e_reset in "60"
do
    mkdir "$e_reset""mV" -v
    cd "$e_reset""mV"

    for current in "50" "100" "150" "200" "300" "400" "500" "600" "700" "800" "900" "1000" "1100" "1200" "1300" "1400" "1500" "1600" "1700" "1800" "1900" "2000"
    do
        mkdir "$current""pA" -v
        cd "$current""pA"
        pushd "$SOURCEDIR"
            final_expr2="set_bg_current(0,""$current""e-3)"
            sed -i "s|set_bg_current(0,.*e-3)|$final_expr2|" TIF.cpp
            make install
        popd
        LD_LIBRARY_PATH=/home/asinha/Documents/02_Code/00_repos/00_mine/herts-research-repo/auryn/src/.libs ~/bin/research-bin/TIFtest ; awk '/^5\./,/^14\./' spikes.ras | wc -l | tr -d '\n' > spikes-in-10.txt; echo '/10' >> spikes-in-10.txt ; cat spikes-in-10.txt | bc -l > firing-rate.txt; awk '/^5\.00/,/^5\.10/' voltages.txt > voltages-1000.txt; gnuplot ../../../../../src/postprocess/scripts/adexgraphs.plt
        cd ..
    done
    for i in *pA; do echo -n "$i " ; cat "$i"/firing-rate.txt; done | sort -n > firing-rates.txt
    cd ..
done

for i in *mV ; do cd "$i" ;  sed "s/pA//" firing-rates.txt > firing-rates-plot.txt; mv firing-rates-plot.txt ../"$i"-firing-rates.txt; cd ..;  done

#gnuplot ../../../src/postprocess/scripts/fi-graphs.plt
