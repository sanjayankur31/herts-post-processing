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

numberofcols=16

for k in k-ee-*; do 
    pushd "$k"

        echo "Processing neuron I ampa"
        let j=0
        finalfile="$k"".neuron_i.ampa-allmerged"
        touch "$finalfile"
        for i in `ls *neuron_i*.ampa | sort -n -t . -k 2`
        do
            if [ "$j" -gt 0 ]
            then
                cut -d " " -f 2- "$i" > "$i"".readytomerge"
            else
                cp "$i" "$i"".readytomerge"
            fi

            paste -d " " "$finalfile" "$i"".readytomerge" > "$finalfile"".new"
            mv "$finalfile"".new" "$finalfile"
            let j=j+1
        done
        #The paste command adds an empty field in its first invocation
        sed -i "s/^ //" "$finalfile"
        echo "Processed neuron I ampa ..."
        
        echo "Processing neuron I gaba"
        let j=0
        finalfile="$k"".neuron_i.gaba-allmerged"
        touch "$finalfile"
        for i in `ls *neuron_i*.gaba | sort -n -t . -k 2`
        do
            if [ "$j" -gt 0 ]
            then
                cut -d " " -f 2- "$i" > "$i"".readytomerge"
            else
                cp "$i" "$i"".readytomerge"
            fi

            paste -d " " "$finalfile" "$i"".readytomerge" > "$finalfile"".new"
            mv "$finalfile"".new" "$finalfile"
            let j=j+1
        done
        #The paste command adds an empty field in its first invocation
        sed -i "s/^ //" "$finalfile"
        echo "Processed neuron I gaba ..."

        echo "Processing neuron E ampa"
        let j=0
        finalfile="$k"".neuron_e.ampa-allmerged"
        touch "$finalfile"
        for i in `ls *neuron_e*.ampa | sort -n -t . -k 2`
        do
            if [ "$j" -gt 0 ]
            then
                cut -d " " -f 2- "$i" > "$i"".readytomerge"
            else
                cp "$i" "$i"".readytomerge"
            fi

            paste -d " " "$finalfile" "$i"".readytomerge" > "$finalfile"".new"
            mv "$finalfile"".new" "$finalfile"
            let j=j+1
        done
        #The paste command adds an empty field in its first invocation
        sed -i "s/^ //" "$finalfile"
        echo "Processed neuron e ampa ..."

        gnuplot -e "set term png font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,30\" size 2048,1024 linewidth 2; set output \"""$k"".conductances.png\"; set title \"Conductances\"; set xlabel \"Time (s)\"; set ylabel \"Conductance (?)\"; plot \"""$k"".neuron_e.ampa-allmerged\" u 1 : ((sum [col=2:""$numberofcols""] column(col))/""$numberofcols"") t \"Ampa conductance - E neurons\" with linespoints, \"""$k"".neuron_i.ampa-allmerged\" u 1 : ((sum [col=2:""$numberofcols""] column(col))/""$numberofcols"") t \"Ampa conductance - I neurons\" with linespoints, \"""$k"".neuron_i.gaba-allmerged\" u 1 : ((sum [col=2:""$numberofcols""] column(col))/""$numberofcols"") t \"Gaba conductance - I neurons\" with linespoints;"

        rm -f *readytomerge *-allmerged

    popd
done
