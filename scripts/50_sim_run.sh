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

for i in `seq 1 10`:
do
    mkdir ~/dump/patternFilesTemp/
    pushd ~/dump/patternFilesTemp/
        Rscript ~/bin/research-scripts/generatePatterns.R
    popd
    # make sure the configuration file is up to date
    ~/bin/research-scripts/run_vogels_mpich.sh
    pushd `tmux show-buffer`
        ~/bin/research-bin/postprocess -o `tmux show-buffer` -c `tmux show-buffer`.cfg -S -s && rm *.ras *.netstate *.weightinfo *.rate *.log -f
    popd
    rm -rf ~/dump/patternFilesTemp/ && sleep 1
done

echo "50 sim run finished. Post process away!"
