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

#PBS -l walltime=24:00:00
#PBS -l nodes=32
#PBS -m abe
#PBS -N "auryn"

module unload mpi/mpich-x86_64
module load mpi/openmpi-x86_64

cd /stri-data/asinha/results/

/stri-data/asinha/herts-research-repo/src/postprocess/scripts/run_vogels_mpich.sh -m 32 -p /stri-data/asinha/herts-research-repo/ -e 10
