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

function run() {
    splitwidth="$2"
    echo "Convering to matrixed form"
    grep -E -o -m 1 "([0-9]+\.[0-9]+ ){$splitwidth}" "$1" > "$1""-lined"
}

if [ "$#" -ne 2 ]; then
    echo "Something went wrong. Exiting."
    usage
else
    run "$1" "$2"
fi

