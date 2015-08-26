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

runs="$1"
for i in `seq 1 $runs`:
do
    mkdir ~/dump/patternFilesTemp/
    pushd ~/dump/patternFilesTemp/
        Rscript ~/bin/research-scripts/generatePatterns.R
    popd
    # make sure the configuration file is up to date
    ~/bin/research-scripts/run_vogels_mpich.sh
    ~/bin/research-scripts/postprocess-mpich.sh -d `tmux show-buffer` -o
    pushd `tmux show-buffer`
        ~/bin/research-bin/postprocess -o `tmux show-buffer` -c `tmux show-buffer`.cfg -S -s -e && rm *.ras *.netstate *.weightinfo *.rate *.log *merge* -f
    popd
    echo `tmux show-buffer` >> dirs-created.txt
    rm -rf ~/dump/patternFilesTemp/ && sleep 1
done

echo "$runs simulations finished. Post process away!"
