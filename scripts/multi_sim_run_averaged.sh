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
# File : multi_sim_run_averaged.sh
#

chis="6"

#averaged_dir="multi-chis-averaged-overwritten-constant-current-1"
#mkdir "$averaged_dir"

runs="$1"
for chi in $(echo $chis)
do
    sed -i "s/chi=.*/chi=$chi/" /home/asinha/Documents/02_Code/00_repos/00_mine/herts-research-repo/src/simulation_config.cfg
    echo "simulation_config.cfg updated."

    combined_dir="chi-""$chi"
    mkdir "$combined_dir"
    cp /home/asinha/Documents/02_Code/00_repos/00_mine/herts-research-repo/src/simulation_config.cfg "$combined_dir"
    #cp /home/asinha/Documents/02_Code/00_repos/00_mine/herts-research-repo/src/simulation_config.cfg "$averaged_dir"

    for i in $(seq 1 "$runs")
    do
        echo "Simulation #$i starting."
        mkdir ~/dump/patternFilesTemp/
        pushd ~/dump/patternFilesTemp/
            Rscript ~/bin/research-scripts/generatePatterns.R
        popd
        # make sure the configuration file is up to date
        ~/bin/research-scripts/run_vogels_mpich.sh
        pushd "$(tmux show-buffer)"
            ~/bin/research-bin/postprocess -o "$(tmux show-buffer)" -c "$(tmux show-buffer)"".cfg" -S -s && rm ./*.ras ./*.netstate ./*.weightinfo ./*.rate ./*.log -f

            prefix=$(printf "%02d" $(($i*1)))
            rename "00" "$prefix" ./*.txt 
            cp "$prefix"* ../"$combined_dir" -v
        popd
        tmux show-buffer >> dirs-created.txt
        echo -n "\t" >> dirs-created.txt
        rm -rf ~/dump/patternFilesTemp/ && sleep 1
    done
    
    pushd "$combined_dir"
        master_snr_file="00-SNR-data-k-w-""$chi"".txt-all"
        master_mean_file="00-Mean-data-k-w-""$chi"".txt-all"
        master_std_file="00-STD-data-k-w-""$chi"".txt-all"
        master_mean_noise_file="00-Mean-noise-data-k-w-""$chi"".txt-all"
        master_std_noise_file="00-STD-noise-data-k-w-""$chi"".txt-all"

        averaged_snr_file="00-SNR-data-k-w-""$chi"".txt"
        averaged_mean_file="00-Mean-data-k-w-""$chi"".txt"
        averaged_std_file="00-STD-data-k-w-""$chi"".txt"
        averaged_mean_noise_file="00-Mean-noise-data-k-w-""$chi"".txt"
        averaged_std_noise_file="00-STD-noise-data-k-w-""$chi"".txt"

        touch "$master_snr_file"
        touch "$master_mean_file"
        touch "$master_std_file"
        touch "$master_mean_noise_file"
        touch "$master_std_noise_file"

        for j in $(ls *SNR-data.txt); do
            paste "$master_snr_file" "$j" > "$master_snr_file""-new"
            mv "$master_snr_file""-new" "$master_snr_file"
        done
        for j in $(ls *Mean-data.txt); do
            paste "$master_mean_file" "$j" > "$master_mean_file""-new"
            mv "$master_mean_file""-new" "$master_mean_file"
        done
        for j in $(ls *Mean-noise-data.txt); do
            paste "$master_mean_noise_file" "$j" > "$master_mean_noise_file""-new"
            mv "$master_mean_noise_file""-new" "$master_mean_noise_file"
        done
        for j in $(ls *STD-data.txt); do
            paste "$master_std_file" "$j" > "$master_std_file""-new"
            mv "$master_std_file""-new" "$master_std_file"
        done
        for j in $(ls *STD-noise-data.txt); do
            paste "$master_std_noise_file" "$j" > "$master_std_noise_file""-new"
            mv "$master_std_noise_file""-new" "$master_std_noise_file"
        done

        sed -e "s/^\t/(/" -e "s/\t/\+/g" -e "s/$/)\/$runs/" "$master_snr_file" > "$master_snr_file""-cmd"
        sed -e "s/^\t/(/" -e "s/\t/\+/g" -e "s/$/)\/$runs/" "$master_mean_file" > "$master_mean_file""-cmd"
        sed -e "s/^\t/(/" -e "s/\t/\+/g" -e "s/$/)\/$runs/" "$master_mean_noise_file" > "$master_mean_noise_file""-cmd"
        sed -e "s/^\t/(/" -e "s/\t/\+/g" -e "s/$/)\/$runs/" "$master_std_file" > "$master_std_file""-cmd"
        sed -e "s/^\t/(/" -e "s/\t/\+/g" -e "s/$/)\/$runs/" "$master_std_noise_file" > "$master_std_noise_file""-cmd"

        bc -l < "$master_snr_file""-cmd" > "$averaged_snr_file"
        bc -l < "$master_mean_file""-cmd" > "$averaged_mean_file"
        bc -l < "$master_mean_noise_file""-cmd" > "$averaged_mean_noise_file"
        bc -l < "$master_std_file""-cmd" > "$averaged_std_file"
        bc -l < "$master_std_noise_file""-cmd" > "$averaged_std_noise_file"

#        cp "$averaged_snr_file" "$averaged_std_file" "$averaged_mean_file" "$averaged_std_noise_file" "$averaged_mean_noise_file" ../"$averaged_dir" -v
        cp "$averaged_snr_file" "00-SNR-data.txt"
        cp "$averaged_mean_file" "00-Mean-data.txt"
        cp "$averaged_mean_noise_file" "00-Mean-noise-data.txt"
        cp "$averaged_std_file" "00-STD-data.txt"
        cp "$averaged_std_noise_file" "00-STD-noise-data.txt"
        ~/bin/research-bin/postprocess -o 'wPats-averaged' -g -k
    popd
done

#pushd "$averaged_dir"
#   ~/bin/research-bin/postprocess -o 'bg_current = 4pA' -k -P
#popd

echo "All done. Hopefully, everything went as expected."
