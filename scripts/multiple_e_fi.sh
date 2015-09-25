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

    #for current in "0" "25" "50" "75" "100" "125" "150" "175" "200" "225" "250" "275" "300" "325" "350" "375" "400" "425" "450" "475" "500" "525" "550" "575" "600" "625" "650" "675" "700" "725" "750" "775" "800" "825" "850" "875" "900" "925" "950" "975" "1000" "1025" "1050" "1075" "1100" "1125" "1150" "1175" "1200" "1225" "1250" "1275" "1300" "1325" "1350" "1375" "1400" "1425" "1450" "1475" "1500" "1525" "1550" "1575" "1600" "1625" "1650" "1675" "1700" "1725" "1750" "1775" "1800" "1825" "1850" "1875" "1900" "1925" "1950" "1975" "2000" "2025" "2050" "2075" "2100" "2125" "2150" "2175" "2200" "2225" "2250" "2275" "2300" "2325" "2350" "2375" "2400" "2425" "2450" "2475" "2500" "2525" "2550" "2575" "2600" "2625" "2650" "2675" "2700" "2725" "2750" "2775" "2800" "2825" "2850" "2875" "2900" "2925" "2950" "2975" "3000" "3025" "3050" "3075" "3100" "3125" "3150" "3175" "3200" "3225" "3250" "3275" "3300" "3325" "3350" "3375" "3400" "3425" "3450" "3475" "3500" "3525" "3550" "3575" "3600" "3625" "3650" "3675" "3700" "3725" "3750" "3775" "3800" "3825" "3850" "3875" "3900" "3925" "3950" "3975" "4000" "4025" "4050" "4075" "4100" "4125" "4150" "4175" "4200" "4225" "4250" "4275" "4300" "4325" "4350" "4375" "4400" "4425" "4450" "4475" "4500" "4525" "4550" "4575" "4600" "4625" "4650" "4675" "4700" "4725" "4750" "4775" "4800" "4825" "4850" "4875" "4900" "4925" "4950" "4975" "5000" "5025" "5050" "5075" "5100" "5125" "5150" "5175" "5200" "5225" "5250" "5275" "5300" "5325" "5350" "5375" "5400" "5425"

for e_reset in "85" "80" "75" "70" "65" "60" "58"
do
    (

    #mkdir "$e_reset""mV" -v
    cd "$e_reset""mV"

    for current in "21775" "21800" "21825" "21850" "21875" "21900" "21925" "21950" "21975" "22000" "22025" "22050" "22075" "22100" "22125" "22150" "22175" "22200" "22225" "22250" "22275" "22300" "22325" "22350" "22375" "22400" "22425" "22450" "22475" "22500" "22525" "22550" "22575" "22600" "22625" "22650" "22675" "22700" "22725" "22750" "22775" "22800" "22825" "22850" "22875" "22900" "22925" "22950" "22975" "23000" "23025" "23050" "23075" "23100" "23125" "23150" "23175" "23200" "23225" "23250" "23275" "23300" "23325" "23350" "23375" "23400" "23425" "23450" "23475" "23500" "23525" "23550" "23575" "23600" "23625" "23650" "23675" "23700" "23725" "23750" "23775" "23800" "23825" "23850" "23875" "23900" "23925" "23950" "23975" "24000" "24025" "24050" "24075" "24100" "24125" "24150" "24175" "24200" "24225" "24250" "24275" "24300" "24325" "24350" "24375" "24400" "24425" "24450" "24475" "24500" "24525" "24550" "24575" "24600" "24625" "24650" "24675" "24700" "24725" "24750" "24775"

    do
        mkdir "$current""pA" -v
        cd "$current""pA"

        cp "$SOURCEDIR"/adextest.cpp .
        cp "$SOURCEDIR"/Makefile .
        final_expr1="set_e_reset(-""$e_reset""e-3)"
        final_expr2="set_bg_current(0,""$current""e-3/g_leak)"
        sed -i -e "s|set_e_reset(-.*e-3)|$final_expr1|" -e "s|set_bg_current(0,.*e-3/g_leak)|$final_expr2|" adextest.cpp
        make adextest

        LD_LIBRARY_PATH=/home/asinha/Documents/02_Code/00_repos/00_mine/herts-research-repo/auryn/build/src/ ./adextest ; awk '/^5\./,/^14\./' spikes.ras | wc -l | tr -d '\n' > spikes-in-10.txt; echo '/10' >> spikes-in-10.txt ; cat spikes-in-10.txt | bc -l > firing-rate.txt; awk '/^5\.00/,/^5\.10/' voltages.txt > voltages-1000.txt; gnuplot ../../../../../src/postprocess/scripts/adexgraphs.plt; mv Adex.png adex-"$e_reset""mV-""$current""pA.png"
        rm -f adextest* Makefile spikes.ras voltages.txt output.log
        cd ..
    done

    for i in *pA; do echo -n "$i " ; cat "$i"/firing-rate.txt; done | sort -n > firing-rates.txt
    cd ..

    ) &
done

wait

for i in *mV ; do cd "$i" ;  sed "s/pA//" firing-rates.txt > firing-rates-plot.txt; mv firing-rates-plot.txt ../"$i"-firing-rates.txt; cd ..;  done

gnuplot "$SCRIPTDIR""fi-graphs.plt"
