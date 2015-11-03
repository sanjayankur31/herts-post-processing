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
# File : multi_sim_run.sh
#

#Default number of RUNS
RUNS=1
PROGRAM_PREFIX="/home/asinha/Documents/02_Code/00_repos/00_mine/herts-research-repo/"
MPI_RANKS="16"
CLUSTER="no"

function default ()
{
    for i in `seq 1 $RUNS`:
    do
        ~/bin/research-scripts/run_vogels_mpich.sh -p "$PROGRAM_PREFIX" -r -s "0.5" -m "$MPI_RANKS" -t -D -P
        echo `tmux show-buffer` >> dirs-created.txt
    done

    echo "[INFO] $RUNS simulations finished and postprocessed."
}

function run()
{
    while getopts "hcn:m:" OPTION
    do
        case $OPTION in
            c) 
                CLUSTER="yes"
                PROGRAM_PREFIX="/stri-data/asinha/herts-research-repo/"
                default
                ;;
            n)
                RUNS=$OPTARG
                ;;
            m)
                MPI_RANKS=$OPTARG
                ;;
            h)
                usage
                exit 1
                ;;
            ?)
                usage
                exit 1
                ;;
        esac
    done
    default
}

# check for tmux
which tmux > /dev/null 2>&1
if [ 0 -ne "$?" ]
then
    echo "[ERROR] Tmux not found on this machine. Will not run. Exiting"
    exit -3
fi

NUMARGS=$#
echo "[INFO] Number of arguments: $NUMARGS"
if [ $NUMARGS -eq 0 ]; then
    echo "[INFO] No arguments received. Running with default configuration. Use -h to see usage help."

    default
    exit 0
else
    run "$@"
fi


run "$@"
