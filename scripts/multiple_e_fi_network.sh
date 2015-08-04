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
SCRIPTDIR="$SOURCEDIR""postprocess/scripts/"
configfile="/home/asinha/Documents/02_Code/00_repos/00_mine/herts-research-repo/src/simulation_config.cfg"
mpi_ranks=$(grep "mpi_ranks" $configfile | sed "s/.*=//")
NE="8000"
NI="2000"

for k_w_ee in "0" "0.01" "0.015" "0.02" "0.025" "0.03" "0.035" "0.04" "0.045" "0.05" "0.055" "0.06" "0.065" "0.07"
#for k_w_ee in "0.063" "0.064" "0.065" "0.066" "0.067" "0.07"
do
    mkdir k-ee-"$k_w_ee" -v
    cd k-ee-"$k_w_ee"
        cp "$configfile" ./"config.cfg" -v
        final_expr2="k_w_ee=""$k_w_ee"
        sed -i -e "s|k_w_ee=.*$|$final_expr2|" "config.cfg"
        LD_LIBRARY_PATH=/home/asinha/Documents/02_Code/00_repos/00_mine/herts-research-repo/auryn/src/.libs mpiexec -n $mpi_ranks ~/bin/research-bin/vogels --out output --config "config.cfg"; 

        sort --parallel=20 -g -m *_e.ras > "spikes-e.ras"
        awk '/^5\./,/^14\./' spikes-e.ras | wc -l | tr -d '\n' > spikes-in-10-e.txt; echo "/(10*$NE)" >> spikes-in-10-e.txt ; cat spikes-in-10-e.txt | bc -l > firing-rate-e.txt; 

        sort --parallel=20 -g -m *_i.ras > "spikes-i.ras"
        awk '/^5\./,/^14\./' spikes-i.ras | wc -l | tr -d '\n' > spikes-in-10-i.txt; echo "/(10*$NI)" >> spikes-in-10-i.txt ; cat spikes-in-10-i.txt | bc -l > firing-rate-i.txt; 
    cd ..
done
for i in k-ee-*; do echo -n "$i " ; cat "$i"/firing-rate-e.txt; done | sort -n > firing-rates-e.txt
for i in k-ee-*; do echo -n "$i " ; cat "$i"/firing-rate-i.txt; done | sort -n > firing-rates-i.txt

sed -i "s/k-ee-//" firing-rates-e.txt 
sed -i "s/k-ee-//" firing-rates-i.txt 

sort -n firing-rates-e.txt > tmp.txt; mv tmp.txt firing-rates-e.txt
sort -n firing-rates-i.txt > tmp.txt; mv tmp.txt firing-rates-i.txt

gnuplot "$SCRIPTDIR""f-w_ee-graphs.plt"
