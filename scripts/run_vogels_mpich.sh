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
# File :  run_vogels_mpich.sh
#
# Wrapper for my simulation.
#

PROGRAM_PREFIX="/home/asinha/Documents/02_Code/00_repos/00_mine/herts-research-repo/"
PATTERNFILES_DIR="$PROGRAM_PREFIX""src/input_files/"
RECALL_RATIO=".50"
MPI_RANKS="16"
POSTPROCESS="no"
POSTPROCESS_MASTER="no"
DELETEDATAFILES="no"
RANDOM_PATTERNS="no"
PATTERNS="3"
RUNSIMULATION="yes"

function usage ()
{
    cat << EOF
usage: $0 options

Wrapper script for my simulation program.

OPTIONS:
    -h  Show this message and exit
    -p  program_prefix (default: $PROGRAM_PREFIX)
    -r  use randomly generated pattern files (default: $RANDOM_PATTERNS)
    -s  ratio of recall neurons (default: $RECALL_RATIO). IMPLIES -r.
    -m  number of mpi ranks to use (default: taken from simulation_config.cfg)
    -P  postprocess to generate SNR graphs (default: $POSTPROCESS)
    -D  delete datafiles after graph generation (default: $DELETEDATAFILES)
    -t  run post process script to generate master graphs (default: $POSTPROCESS_MASTER)
    -e  number of patterns (default: $PATTERNS)
    -N  skip running simulation and only postprocess files in a given directory - implies P - (absolute paths only)
EOF

}
function default()
{
    echo "[INFO] Program prefix is: $PROGRAM_PREFIX"

    if [ "xyes" == "x$RUNSIMULATION" ]
    then

        SIM_DIRECTORY=$(date "+%Y%m%d%H%M")
        SIM_DIRECTORY_COMPLETE="$(pwd)""/""$SIM_DIRECTORY"
        mkdir "$SIM_DIRECTORY_COMPLETE"

        pushd "$SIM_DIRECTORY_COMPLETE" > /dev/null 2>&1

            # setup for random pattern files
            if [ "xyes" == "x$RANDOM_PATTERNS" ]
            then
                PATTERNFILES_DIR="./00_patternfiles/"
                echo "[INFO] Generating random pattern and recall files with recall ratio of $RECALL_RATIO in $PATTERNFILES_DIR"
                mkdir "$PATTERNFILES_DIR"
                pushd "$PATTERNFILES_DIR" > /dev/null 2>&1
                    cp "$PROGRAM_PREFIX""/src/postprocess/scripts/generatePatterns.R" .
                    sed -i "s|recallPercentOfPattern <- 0.50|recallPercentOfPattern <- $RECALL_RATIO|" generatePatterns.R
                    Rscript generatePatterns.R
                popd > /dev/null 2>&1
                PATTERNFILE_PREFIX="$PATTERNFILES_DIR""randomPatternFile-"
                RECALLFILE_PREFIX="$PATTERNFILES_DIR""recallPatternFile-"

            else
                echo "[INFO] Random patterns not used, default recall ratio of .05 used"
                echo "[INFO] Pattern files used from $PATTERNFILES_DIR""50-percent/"

                PATTERNFILE_PREFIX="$PATTERNFILES_DIR""50-percent/randomPatternFile-"
                RECALLFILE_PREFIX="$PATTERNFILES_DIR""50-percent/recallPatternFile-"
            fi

            # the actual simulation
            CONFIGFILE="$PROGRAM_PREFIX""src/simulation_config.cfg"
            echo "[INFO] MPI ranks being used: $MPI_RANKS"

            if [ "xyes" == "x$SAVE_TMUX" ]
            then
                tmux set-buffer "$SIM_DIRECTORY"
                echo "[INFO] Saved in tmux buffer for your convenience."
            fi

            cp "$CONFIGFILE" ./$SIM_DIRECTORY.cfg
            echo "patternfile_prefix=$PATTERNFILE_PREFIX" >> "$SIM_DIRECTORY"".cfg"
            echo "recallfile_prefix=$RECALLFILE_PREFIX" >> "$SIM_DIRECTORY"".cfg"
            echo "mpi_ranks=$MPI_RANKS" >> "$SIM_DIRECTORY"".cfg"
            echo "num_pats=$PATTERNS" >>  "$SIM_DIRECTORY"".cfg"
            echo >> "$SIM_DIRECTORY"".cfg"
            echo "#recall_ratio=$RECALL_RATIO" >> "$SIM_DIRECTORY"".cfg"

            echo 
            echo "[INFO] $ LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PROGRAM_PREFIX""auryn/build/src/ mpirun -n $MPI_RANKS $PROGRAM_PREFIX""bin/vogels --out $SIM_DIRECTORY" "--config $SIM_DIRECTORY"".cfg"
            echo 

            LD_LIBRARY_PATH="$LD_LIBRARY_PATH:""$PROGRAM_PREFIX""auryn/build/src/" mpirun -n $MPI_RANKS "$PROGRAM_PREFIX""bin/vogels" --out $SIM_DIRECTORY --config $SIM_DIRECTORY".cfg" 

        popd > /dev/null 2>&1
    else
        SIM_DIRECTORY=$(echo $RUNSIMULATION | sed -e 's|\.||g' -e 's|/||g')
        SIM_DIRECTORY_COMPLETE="$(pwd)""/"$SIM_DIRECTORY
    fi

    if [ "xyes" == "x$POSTPROCESS" ]
    then
        pushd "$SIM_DIRECTORY_COMPLETE" > /dev/null 2>&1

            MPI_RANKS=$(grep "mpi_ranks=" $SIM_DIRECTORY.cfg | sed "s|mpi_ranks=||")
            echo "[INFO] Postprocessing generated data files."

            echo "[INFO] Generating combined connection files for postprocessing"
            sed -i '/^%/ d' 00-Con_* # get rid of useless headers
            sort -g -m 00-Con_ee* > 00-Con_ee.txt
            sort -g -m 00-Con_ie* > 00-Con_ie.txt

            sed '/^%/ d ' 00-Con_ee.txt |  cut -f2 -d " " |sort -n | uniq -c > 00-uniq-ee.txt
            sed '/^%/ d ' 00-Con_ie.txt |  cut -f2 -d " " |sort -n | uniq -c > 00-uniq-ie.txt
            sed '/^%/ d ' 00-Con_ee.txt |  cut -f2 -d " " > 00-all-ee.txt
            sed '/^%/ d ' 00-Con_ie.txt |  cut -f2 -d " " > 00-all-ie.txt
            wc -l 00-uniq* 00-all* > 00-conn-stats.txt

            # Run the post process program that processes data from each rank
            # and prints it out along with my histograms
            mpirun -n $MPI_RANKS "$PROGRAM_PREFIX""bin/postprocess_single" -o "$SIM_DIRECTORY" -c "$SIM_DIRECTORY".cfg

            # Combine SNR data from different ranks
            echo "[INFO] Generating combined SNR, STD, Mean, Mean-noise, STD-noise files for postprocessing"
            sort -g -m 00-SNR-data.*.txt | cut -f2 > 00-SNR-data.txt
            sort -g -m 00-STD-data.*.txt | cut -f2 > 00-STD-data.txt
            sort -g -m 00-Mean-data.*.txt | cut -f2 > 00-Mean-data.txt
            sort -g -m 00-Mean-noise-data.*.txt | cut -f2 > 00-Mean-noise-data.txt
            sort -g -m 00-STD-noise-data.*.txt | cut -f2 > 00-STD-noise-data.txt

            # Generate SNR graphs
            "$PROGRAM_PREFIX""/bin/grapher_single" -c $SIM_DIRECTORY.cfg -o $SIM_DIRECTORY
        popd > /dev/null 2>&1

    fi

    if [ "xyes" == "x$POSTPROCESS_MASTER" ]
    then
        "$PROGRAM_PREFIX""/src/postprocess/scripts/postprocess-mpich.sh" -d "$SIM_DIRECTORY" -o
    fi

    if [ "xyes" == "x$DELETEDATAFILES" ]
    then
        pushd "$SIM_DIRECTORY_COMPLETE" > /dev/null 2>&1
            rm *.ras *.weightinfo *.rate *.log *merge* *bras 00*.*.txt -f
            rm -f 00-Con_ee.*.txt 00-Con_ie.*.txt
            rm 00-SNR-data.*.txt 00-STD-data.*.txt 00-Mean-data.*.txt 00-Mean-noise-data.*.txt 00-STD-noise_data.*.txt -f
        popd > /dev/null 2>&1
    fi
    rm -f *.netstate 

    echo "Result directory is: $SIM_DIRECTORY_COMPLETE"
    exit 0
}

function run()
{
    while getopts "hp:rs:m:tPDe:N:" OPTION
    do
        case $OPTION in
            p)
                PROGRAM_PREFIX=$OPTARG
                ;;
            r)
                RANDOM_PATTERNS="yes"
                ;;
            s)
                RECALL_RATIO=$OPTARG
                RANDOM_PATTERNS="yes"
                ;;
            m) 
                MPI_RANKS=$OPTARG
                ;;
            t)
                POSTPROCESS_MASTER="yes"
                ;;
            P)
                POSTPROCESS="yes"
                ;;
            D)
                DELETEDATAFILES="yes"
                ;;
            e)
                PATTERNS=$OPTARG
                ;;
            N)
                RUNSIMULATION=$OPTARG
                POSTPROCESS="yes"
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

    which gnuplot > /dev/null 2>&1
    if [ 0 -ne "$?" ]
    then
        echo "[ERROR] Gnuplot not found on this machine. Will not postprocess and will not delete files."
        POSTPROCESS="no"
        POSTPROCESS_MASTER="no"
        DELETEDATAFILES="no"
    fi

    default
}

# MAIN CALLS

# check for tmux
which tmux > /dev/null 2>&1
if [ 0 -ne "$?" ]
then
    SAVE_TMUX="no"
    echo "[INFO] Tmux not found on this machine. Not saving directory name"
else
    SAVE_TMUX="yes"
    echo "[INFO] Tmux found on this machine. Saving directory name"
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

