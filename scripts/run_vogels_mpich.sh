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

function usage ()
{
    cat << EOF
usage: $0 options

Wrapper script for my simulation program.

OPTIONS:
    -h  Show this message and exit
EOF

}
function default()
{
    newsimenv=$(date "+%Y%m%d%H%M")
    configfile="/home/asinha/Documents/02_Code/00_repos/00_mine/herts-research-repo/src/simulation_config.cfg"
    mkdir -v "$newsimenv"
    pushd "$newsimenv"
        cp "$configfile" . -v
        echo "$ mpiexec -n 16 vogels --out $newsimenv"
        LD_LIBRARY_PATH=/home/asinha/Documents/02_Code/00_repos/00_mine/herts-research-repo/auryn/src/.libs mpiexec -n 16 ~/bin/research-bin/vogels --out $newsimenv
    popd
    echo "Result directory is: $newsimenv"
    tmux set-buffer "$newsimenv"
    echo "Saved in tmux buffer for your convenience."
}


function run()
{
    while getopts "h" OPTION
    do
        case $OPTION in
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


run "$@"
