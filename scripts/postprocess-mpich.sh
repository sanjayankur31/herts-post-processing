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
# File : postprocess-mpich.sh
#
# All the post processing required to generate graphs for my simulation
#

SCRIPTDIR="/home/asinha/Documents/02_Code/00_repos/00_mine/auryn/scripts/"
timestamp=
cleandata=
overall=
collectoveralldata=
surfaceplottimes=
collectdata=

function usage ()
{
    cat << EOF
    usage: $0 options

    Master script for all postprocessing for my simulations. Tread lightly!

    -d  directory to work in
    -s  plot surface plots
    -o  plot overall time plot
    -S  plot surface plots - do not recollect data - assumes data is already processed in R
    -O  plot overall time plot only - does not recollect data - assumes data is already processed in R
    -c  clean dir of all generated text files
    -C  clean dir of all generated files including graphs
    -h  print this message and exit

    NOTE: Must use flags repeatedly for bash getopts.
EOF
}



function run ()
{
    while getopts "hd:sSoOcC" OPTION
    do
        case $OPTION in
            h)
                usage
                exit 1
                ;;
            d)
                timestamp="$OPTARG"
                ;;
            o)
                overall="yes"
                collectoveralldata="yes"
                ;;
            O)
                overall="yes"
                collectoveralldata="no"
                ;;
            s)
                surfaceplottimes="yes"
                collectdata="yes"
                ;;
            S)
                surfaceplottimes+="yes"
                collectdata="no"
                ;;
            c)
                cleandata="temp"
                ;;
            C)
                cleandata="all"
                ;;
            ?)
                usage
                exit 1
                ;;
        esac
    done

    pushd "$timestamp"
        (
        if [[ "$surfaceplottimes" == "yes" ]] && [[ "$collectdata" == "yes" ]]
        then
            echo  "Generating surface graphs."
            # combine rank files
            if [[ ! -e "RAS-COMBINED" ]]
            then
                collectdata
            else
                echo "Ras files are already combined. Calling R now."
            fi

            # Go all R on this data!
            echo "******* Passing on information to postprocess"

            echo "******* Running: postprocess -o $timestamp -e $timestamp"".e.ras -i $timestamp"".i.ras"
            ~/bin/research-bin/postprocess "-o" "$timestamp" "-e" "$timestamp"".e.ras" "-i" "$timestamp"".i.ras"
            echo

            for matrixFile in `ls *.combined.matrix`
            do
                datafile=$(basename "$matrixFile" ".combined.matrix")
                sanitycheck
                echo
                plotspecifictimeplots
                echo
            done
        elif [[ "$surfaceplottimes" == "yes" ]] && [[ "$collectdata" == "no" ]]
        then
            echo  "Generating surface graphs."
            echo "***** Assuming files already merged. Just plotting with gnuplot."
            for matrixFile in `ls *.combined.matrix`
            do
                datafile=$(basename "$matrixFile" ".combined.matrix")
                sanitycheck
                echo
                plotspecifictimeplots
                echo
            done
        fi
        ) &
        (
        if [[ "$overall" == "yes" ]] && [[ "$collectoveralldata" == "yes" ]]
        then
            echo  "Generating overall graph."
            if [[ ! -e "OVERALL-PLOTTED" ]]
            then
                collectoveralldata
            else
                echo "Processing seems to already have been done. Just replotting."
            fi

            sanitycheckoverall
            plotoverallplots
            touch "OVERALL-PLOTTED"
        elif [[ "$overall" == "yes" ]] && [[ "$collectoveralldata" == "no" ]]
        then
            echo  "Generating overall graph."
            echo "**** Assuming data already collected. Replotting only. *****"
            sanitycheckoverall
            plotoverallplots
            touch "OVERALL-PLOTTED"
        fi
        ) &
        wait
        if [[ "$cleandata" == "temp" ]]
        then
            echo "Deleting all tempfiles. Leaving graphs."
            rm -fv *-allmerged *-multiline *-matrix *-single *readytomerge *.e.ras *.i.ras *.combined.matrix OVERALL-PLOTTED RAS-COMBINED
        elif [[ "$cleandata" == "all" ]]
        then
            echo "Deleting all generated files including graphs."
            rm -fv *-allmerged *-multiline *-matrix *-single *readytomerge *.e.ras *.i.ras *.combined.matrix OVERALL-PLOTTED *.png RAS-COMBINED
        fi
    popd
    # Not staging to git. Appears to cause some issue - images aren't added somehow.
    echo "Output in $timestamp"
    exit 0
}


function collectdata()
{
    if [ -e "$timestamp"".e.ras" ]
    then
        echo "Ras file already exists. Skipping sort and merge comands."
    else
        sort --parallel=12 -g -m "$timestamp"*_e.ras > "$timestamp"".e.ras" &
        sort --parallel=12 -g -m "$timestamp"*_i.ras > "$timestamp"".i.ras" &
        wait
        echo "Ras files combined."
    fi
    touch "RAS-COMBINED"
}

function sanitycheck()
{
    echo "Checking rows and columns in generated files."
    echo "For Excitatory files" 
    finalfile="$datafile"".e.rate-multiline"
    echo "** $finalfile **"
    echo "Number of lines: `wc -l $finalfile | cut -d ' ' -f 1`"
    echo "Number of fields: `awk '{print NF; exit}' $finalfile`"

    echo "For Inhibitory files" 
    finalfile="$datafile"".i.rate-multiline"
    echo "** $finalfile **"
    echo "Number of lines: `wc -l $finalfile | cut -d ' ' -f 1`"
    echo "Number of fields: `awk '{print NF; exit}' $finalfile`"

    finalfile="$datafile"".combined.matrix"
    echo "** $finalfile **"
    echo "Number of lines: `wc -l $finalfile | cut -d ' ' -f 1`"
    echo "Number of fields: `awk '{print NF; exit}' $finalfile`"

}

function collectoveralldata()
{

    # remove the time from the second file onwards only. Leave it be in the first file for plotting
    # Excitatory first
    # It's OK to use rate output for average rates since order doesn't matter.
    ( 
    echo "Processing excitatory rate files ..."
    let j=0
    finalfile="$timestamp"".e.rate-allmerged"
    touch "$finalfile"
    for i in `ls *_e.rate | sort -n -t . -k 2`
    do
        if [ "$j" -gt 0 ]
        then
            cut -d " " -f 2- "$i" > "$i"".readytomerge"
        else
            cp "$i" "$i"".readytomerge"
        fi

        paste -d " " "$finalfile" "$i"".readytomerge" > "$finalfile"".new"
        mv "$finalfile"".new" "$finalfile"
        let j=j+1
    done
    #The paste command adds an empty field in its first invocation
    sed -i "s/^ //" "$finalfile"
    echo "Processed excitatory rate files ..."
    ) &

    (
    # Inhibitory now
    echo "Processing inhibitory rate files ..."
    let j=0
    finalfile="$timestamp"".i.rate-allmerged"
    touch "$finalfile"
    for i in `ls *_i.rate | sort -n -t . -k 2`
    do
        if [ "$j" -gt 0 ]
        then
            cut -d " " -f 2- "$i" > "$i"".readytomerge"
        else
            cp "$i" "$i"".readytomerge"
        fi;

        paste -d " " "$finalfile" "$i"".readytomerge" > "$finalfile"".new"
        mv "$finalfile"".new" "$finalfile"
        let j=j+1;
    done
    #The paste command adds an empty field in its first invocation
    sed -i "s/^ //" "$finalfile"
    echo "Processed inhibitory files ..."
    ) &

    (
    # STDP weight sum
    echo "Processing weight files ..."
    for exts in "ie_stdp.weightinfo" "ii_static.weightinfo" "ee_static.weightinfo" "ei_static.weightinfo" "exte_static.weightinfo"
    do
        wildcard="*_""$exts"
        if ls $wildcard 1> /dev/null 2>&1; 
        then
            let j=0
            finalfile="$timestamp"".""$exts""-allmerged"
            touch "$finalfile"
            for i in `ls $wildcard | sort -n -t . -k 2`
            do
                if [ "$j" -gt 0 ]
                then
                    cut -d " " -f 2 "$i" > "$i"".readytomerge"
                else
                    cut -d " " -f 1-2 "$i" > "$i"".readytomerge"
                fi;

                paste -d " " "$finalfile" "$i"".readytomerge" > "$finalfile"".new"
                mv "$finalfile"".new" "$finalfile"
                let j=j+1;
            done
            #The paste command adds an empty field in its first invocation
            sed -i "s/^ //" "$finalfile"
            echo "Processed weight files ..."
        else
            echo "No weight info files found. Skipping this step."
        fi
    done
    ) &
    wait
}

function sanitycheckoverall()
{
    echo "Checking rows and columns in generated files."
    finalfile="$timestamp"".e.rate-allmerged"
    echo "** $finalfile **"
    echo "Number of lines: `wc -l $finalfile | cut -d ' ' -f 1`"
    echo "Number of fields: `awk '{print NF; exit}' $finalfile`"

    finalfile="$timestamp"".i.rate-allmerged"
    echo "** $finalfile **"
    echo "Number of lines: `wc -l $finalfile | cut -d ' ' -f 1`"
    echo "Number of fields: `awk '{print NF; exit}' $finalfile`"

    for exts in "ie_stdp.weightinfo" "ii_static.weightinfo" "ee_static.weightinfo" "ei_static.weightinfo" "exte_static.weightinfo"
    do
        wildcard="*_""$exts"
        if ls $wildcard 1> /dev/null 2>&1; 
        then
            finalfile="$timestamp"".""$exts""-allmerged"
            echo "** $finalfile **"
            echo "Number of lines: `wc -l $finalfile | cut -d ' ' -f 1`"
            echo "Number of fields: `awk '{print NF; exit}' $finalfile`"
        fi
    done
}

function plotspecifictimeplots ()
{
    minErate=$(sort -g "$datafile"".e.rate-multiline" | head -1)
    minIrate=$(sort -g "$datafile"".i.rate-multiline" | head -1)
    maxErate=$(sort -g "$datafile"".e.rate-multiline" | tail -1)
    maxIrate=$(sort -g "$datafile"".i.rate-multiline" | tail -1)
    uniqValsE=$(uniq "$datafile"".e.rate-multiline" | wc -l)
    uniqValsI=$(uniq "$datafile"".i.rate-multiline" | wc -l)
    binwidthE=$(echo "($maxErate-$minErate)/$uniqValsE" | bc -l)
    binwidthI=$(echo "($maxIrate-$minIrate)/$uniqValsI" | bc -l)
    echo "maxErate: $maxErate"
    echo "minErate: $minErate"
    echo "binwidthE: $binwidthE"
    echo "maxIrate: $maxIrate"
    echo "minIrate: $minIrate"
    echo "binwidthI: $binwidthI"
    



    # Figure 4 matrices
    #gnuplot -e "set pm3d map corners2color c1; set term png font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,20\" size 1440,1440; set output \"""$datafile"".ratematrix.png\"; set title \"Firing rates of neurons in the network at time ""$datafile""\"; set xlabel \"neurons\"; set ylabel \"neurons\"; set ytics (\"100\" 0,\"50\" 50,\"0\" 100); splot \"""$datafile"".merged.rate\" matrix"
    gnuplot -e "set view map; set term png font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,20\" size 1440,1440; set output \"""$datafile"".ratematrix.png\"; set title \"Firing rates of neurons in the network at time ""$datafile""\"; set xlabel \"neurons\"; set ylabel \"neurons\"; set ytics (\"100\" 0,\"50\" 50,\"0\" 100); splot \"""$datafile"".combined.matrix\" matrix with image" &
    echo "******* Complete. Generated $datafile"".png."

    #Histograms with firing rates for a particular time t
    if [[ $minErate -eq 0 ]] && [[ $maxErate -eq 0 ]]
    then
        echo "No data to plot for excitatory histogram - skipping."
    else
        gnuplot -e "set style fill solid 0.5; set tics out nomirror; set xrange [""$minErate"":""$maxErate""]; set yrange [0:]; set xlabel \"firing rate\"; set ylabel \"number of neurons\"; binwidth=""$binwidthE""; bin(x,width)=width*floor(x/width)+width/2.0;  set term png font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,20\" size 1440,1440; set output \"""$datafile"".e.histogram.png\"; set title \"Histogram of firing rates at time ""$datafile""\"; plot \"""$datafile"".e.rate-multiline\" using (bin(\$1,binwidth)):(1.0) smooth freq with boxes lc rgb \"blue\" t \"Excitatory\"" &
        echo "******* Complete. Generated $datafile"".e.histogram.png"
    fi

    if [[ $minIrate -eq 0 ]] && [[ $maxIrate -eq 0 ]]
    then
        echo "No data to plot for inhibitory histogram - skipping."
    else
        gnuplot -e "set style fill solid 0.5; set tics out nomirror; set xrange [""$minIrate"":""$maxIrate""]; set yrange [0:]; set xlabel \"firing rate\"; set ylabel \"number of neurons\"; binwidth=""$binwidthI""; bin(x,width)=width*floor(x/width)+width/2.0;  set term png font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,20\" size 1440,1440; set output \"""$datafile"".i.histogram.png\"; set title \"Histogram of inhibitory firing rates at time ""$datafile""\"; plot \"""$datafile"".i.rate-multiline\" using (bin(\$1,binwidth)):(1.0) smooth freq with boxes lc rgb \"red\" t \"Inhibitory\"" &
        echo "******* Complete. Generated $datafile"".i.histogram.png"
    fi
    wait
}

function plotoverallplots ()
{

    numberofcolsE=$(awk '{print NF; exit}' "$timestamp"".e.rate-allmerged")
    numberofrowsE=$(wc -l "$timestamp"".e.rate-allmerged" | cut -d " " -f 1)
    numberofcolsI=$(awk '{print NF; exit}' "$timestamp"".i.rate-allmerged")
    numberofrowsI=$(wc -l "$timestamp"".i.rate-allmerged" | cut -d " " -f 1)
    # All the weight files should have the same number of columns. Do I need to check this?
    # TODO: modify the command after checking individual files. I could probably use if else to append commands but it'll make the script a lot more messy. I should still look into it to prevent future confusions
    numberofcolsSTDP=$(awk '{print NF; exit}' "$timestamp"".ie_stdp.weightinfo-allmerged")

    if ls *_ie_stdp.weightinfo 1> /dev/null 2>&1; 
    then
        if [[ "$numberofrowsE" -ne "$numberofrowsI" ]]
        then 
            echo "Number of rows in E and I files aren't the same. Check input!"
            echo "E has $numberofrowsE rows while I has $numberofrowsI!"
        else
            #Firing rate evolution throughout the simulation
            gnuplot -e "set term png font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,20\" size 1440,1440; set output \"""$timestamp"".timegraphwithweights.png\"; set size 1,1; set multiplot; set title \"Evolution of firing rates\"; set xlabel \"Time (s)\"; set ylabel \"Firing rates (Hz)\"; set origin 0.0, 0.5; set size 1,0.5; plot \"""$timestamp"".e.rate-allmerged\" u 1 : ((sum [col=2:""$numberofcolsE""] column(col))/""$numberofcolsE"") lc rgb \"blue\" t \"Average Excitatory firing rate (Hz)\" with lines, \"""$timestamp"".i.rate-allmerged\" u 1 : ((sum [col=2:""$numberofcolsI""] column(col))/""$numberofcolsI"") lc rgb \"red\" t \"Average Inhibitory Firing rate (Hz)\" with lines; set origin 0.0,0.0; set size 1,0.5; set ylabel \"Synaptic weight (nS)\"; set title \"Evolution of synapses\"; plot \"""$timestamp"".ie_stdp.weightinfo-allmerged\" u 1 : ((sum [col=2:""$numberofcolsSTDP""] column(col))/""$numberofcolsSTDP"") lc rgb \"brown\" t \"Average IE (plastic)\" with lines, \"""$timestamp"".ii_static.weightinfo-allmerged\" u 1 : ((sum [col=2:""$numberofcolsSTDP""] column(col))/""$numberofcolsSTDP"")  t \"Average II (static)\" with lines, \"""$timestamp"".ee_static.weightinfo-allmerged\" u 1 : ((sum [col=2:""$numberofcolsSTDP""] column(col))/""$numberofcolsSTDP"")  t \"Average EE (static)\" with lines, \"""$timestamp"".ei_static.weightinfo-allmerged\" u 1 : ((sum [col=2:""$numberofcolsSTDP""] column(col))/""$numberofcolsSTDP"") t \"Average EI (static)\" with lines, \"""$timestamp"".exte_static.weightinfo-allmerged\" u 1 : ((sum [col=2:""$numberofcolsSTDP""] column(col))/""$numberofcolsSTDP"") t \"Average EXTE (static)\" with lines" &

            echo "Complete. Generated $timestamp.timegraphwithweights.png"
        fi
    else
        echo "No weight files found, not plotting weights."
        if [[ "$numberofrowsE" -ne "$numberofrowsI" ]]
        then 
            echo "Number of rows in E and I files aren't the same. Check input!"
            echo "E has $numberofrowsE rows while I has $numberofrowsI!"
        else
            gnuplot -e "set term png font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,20\" size 1440,1440; set output \"""$timestamp"".timegraphwithoutweights\"; set title \"Evolution of excitatory and inhibitory firing rates\"; set xlabel \"Time (s)\"; set ylabel \"\"; plot \"""$timestamp"".e.rate-allmerged\" u 1 : ((sum [col=2:""$numberofcolsE""] column(col))/""$numberofcolsE"") lc rgb \"blue\" t \"Average Excitatory firing rate (Hz)\" with lines, \"""$timestamp"".i.rate-allmerged\" u 1 : ((sum [col=2:""$numberofcolsI""] column(col))/""$numberofcolsI"") lc rgb \"red\" t \"Average Inhibitory Firing rate (Hz)\" with lines" &
            echo "Complete. Generated $timestamp.rategraph.png"
        fi
    fi
    wait
}

run "$@"

