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
# File : take the firing rates file and format it so that we can plot fig 4
#

function usage () {
    echo "rate2plot_e.sh - take firing rates file and format it so that we can plot fig 4"
    echo "Usual split width for I = 20; E = 80"
    echo "usage - rate2plot.sh <input rate file> <split width>"

    echo "Once done:"
    echo "paste -d \" \" file1 file2> merged.rate"
    echo "Gnuplot commands:"
    echo "> set pm3d map"
    echo "> set pm3d interpolate 0,0"
    echo "> set term png"
    echo "> set output filename.eps"
    echo "> splot \"merged.rate\" matrix"
}

function run () {
    originalFilename="$1"
    splitwidth="$2"
    filename=$(basename "$originalFilename" ".rate")
    removedColName="$filename""-no-col.rate"
    completePath="../""$removedColName"
    splitPrefix="$filename""_"
    splitDir="$filename""-splitfiles"

    # remove the first column
    cut -d " " -f 2- "$originalFilename" > "$removedColName"

    splitfilesandconverttomatrix

}

function splitfilesandconverttomatrix ()
{
    # make a new dir
    mkdir -v "$splitDir"

    pushd "$splitDir"
        # split the file into different lines
        echo "Splitting into separate files for each time iteration"
        split -l 1 -d --additional-suffix=.pertime.rate "$completePath" "$splitPrefix"
        echo "Splitting complete."

        echo "Convering each file to matrixed form"
        for i in *.pertime.rate; do
            grep -E -o -m 1 "([0-9]+\.[0-9]+ ){$splitwidth}" "$i" > "$i""-lined"
        done
    popd

    echo "Now decide what times you want to merge and use the following command to merge them:"
    echo "paste -d \" \" file1 file2> merged.rate"
    echo "Gnuplot commands:"
    echo "> set pm3d map"
    echo "> set pm3d interpolate 0,0"
    echo "> splot \"merged.rate\" matrix"
}

if [ "$#" -ne 2 ]; then
    echo "Something went wrong. Exiting."
    usage
else
    run "$1" "$2"
fi

