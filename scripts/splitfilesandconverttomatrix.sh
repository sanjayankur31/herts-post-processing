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
# File : 
#

function run () {
    originalFilename="$1"
    splitwidth="$2"
    filename=$(basename "$originalFilename" ".rate")
    completePath="../""$originalFilename"
    splitPrefix="$filename""_"
    splitDir="$filename""-splitfiles"

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
}

if [ "$#" -ne 2 ]; then
    echo "Something went wrong. Exiting."
else
    run "$1" "$2"
fi

