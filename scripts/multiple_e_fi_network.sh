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

for k_w_ee in "0"  "0.010" "0.015" "0.020" "0.025" "0.030" "0.035" "0.040" "0.045" "0.050" "0.055" "0.060" "0.065" "0.070" "0.075" "0.080" "0.085" "0.090" "0.095" "0.10" 
    
    #"0.115" "0.12" "0.125" "0.13" "0.135" "0.140" "0.145" "0.150" "0.155" "0.16" "0.165" "0.170" "0.175" "0.180" "0.185" "0.190" "0.195" "0.20" "0.21" "0.215" "0.22" "0.225" "0.23" "0.235" "0.240" "0.245" "0.250" "0.255" "0.26" "0.265" "0.27" "0.275" "0.280" "0.285" "0.290" "0.295" "0.30" "0.31" "0.315" "0.32" "0.325" "0.33" "0.335" "0.340" "0.345" "0.350" "0.355" "0.36" "0.365" "0.37" "0.375" "0.380" "0.385" "0.390" "0.395" "0.40" "0.41" "0.415" "0.42" "0.425" "0.43" "0.435" "0.440" "0.445" "0.450" "0.455" "0.46" "0.465" "0.47" "0.475" "0.480" "0.485" "0.490" "0.495" "0.50" "0.51" "0.515" "0.52" "0.525" "0.53" "0.535" "0.540" "0.545" "0.550" "0.555" "0.56" "0.565" "0.57" "0.575" "0.580" "0.585" "0.590" "0.595" "0.60" "0.61" "0.615" "0.62" "0.625" "0.63" "0.635" "0.640" "0.645" "0.650" "0.655" "0.66" "0.665" "0.67" "0.675" "0.680" "0.685" "0.690" "0.695" "0.70" "0.71" "0.715" "0.72" "0.725" "0.73" "0.735" "0.740" "0.745" "0.750" "0.755" "0.76" "0.765" "0.77" "0.775" "0.780" "0.785" "0.790" "0.795" "0.80" "0.81" "0.815" "0.82" "0.825" "0.83" "0.835" "0.840" "0.845" "0.850" "0.855" "0.86" "0.865" "0.87" "0.875" "0.880" "0.885" "0.890" "0.895" "0.90" "0.91" "0.915" "0.92" "0.925" "0.93" "0.935" "0.940" "0.945" "0.950" "0.955" "0.96" "0.965" "0.97" "0.975" "0.980" "0.985" "0.990" "0.995" "1.00"
do
    mkdir k-ee-"$k_w_ee" -v
    cd k-ee-"$k_w_ee"
        cp "$configfile" ./"config.cfg" -v
        final_expr2="k_w_ee=""$k_w_ee"
        sed -i -e "s|k_w_ee=.*$|$final_expr2|" "config.cfg"
        LD_LIBRARY_PATH=/home/asinha/Documents/02_Code/00_repos/00_mine/herts-research-repo/auryn/build/src/ mpiexec -n $mpi_ranks ~/bin/research-bin/vogels --out k-ee-"$k_w_ee" --config "config.cfg"; 

        sort --parallel=20 -g -m *_e.ras > "spikes-e.ras"
        awk '/^5\./,/^14\./' spikes-e.ras | wc -l | tr -d '\n' > spikes-in-10-e.txt; echo "/(10*$NE)" >> spikes-in-10-e.txt ; cat spikes-in-10-e.txt | bc -l > firing-rate-e.txt; 

        sort --parallel=20 -g -m *_i.ras > "spikes-i.ras"
        awk '/^5\./,/^14\./' spikes-i.ras | wc -l | tr -d '\n' > spikes-in-10-i.txt; echo "/(10*$NI)" >> spikes-in-10-i.txt ; cat spikes-in-10-i.txt | bc -l > firing-rate-i.txt; 

        rm *.ras -f

    cd ..
    #~/bin/research-scripts/postprocess-mpich.sh -d k-ee-"$k_w_ee" -o
    rm k-ee-"$k_w_ee"/*.rate* k-ee-"$k_w_ee"/*.weight* k-ee-"$k_w_ee"/*log -f

done
for i in k-ee-*; do echo -n "$i " ; cat "$i"/firing-rate-e.txt; done | sort -n > firing-rates-e.txt
for i in k-ee-*; do echo -n "$i " ; cat "$i"/firing-rate-i.txt; done | sort -n > firing-rates-i.txt

sed -i "s/k-ee-//" firing-rates-e.txt 
sed -i "s/k-ee-//" firing-rates-i.txt 

sort -n firing-rates-e.txt > tmp.txt; mv tmp.txt firing-rates-e.txt
sort -n firing-rates-i.txt > tmp.txt; mv tmp.txt firing-rates-i.txt

gnuplot "$SCRIPTDIR""f-w_ee-graphs.plt"
