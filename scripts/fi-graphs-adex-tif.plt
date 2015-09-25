set terminal pngcairo size 1536,768;
set output "AdEx-ab0-TIF-FI.png";
#set terminal epslatex size 15cm,7.5cm color newstyle
#set output "AdEx-ab0-TIF-FI.tex";

set border 3;
set xlabel "Input current (pA)";
set ylabel "Firing rate in (Hz)";
set xrange [0:];
set xtics nomirror 0,1000
set ytics nomirror
set key left
#set title "F-I curve for AdEx with ab=0 and different reset voltages and TIF"
#plot "58mV-firing-rates.txt" title "AdEx TS -58mV" with lines lw 2, "60mV-firing-rates.txt" title "AdEx TS -60mV" with lines lw 2, "65mV-firing-rates.txt" title "AdEx TS -65mV" with lines lw 2, "70mV-firing-rates.txt" title "AdEx TS -70mV" with lines lw 2, "75mV-firing-rates.txt" title "AdEx TS -75mV" with lines lw 2, "80mV-firing-rates.txt" title "AdEx TS -80mV" with lines lw 2, "85mV-firing-rates.txt" title "AdEx TS -85mV" with lines lw 2, "60mV-firing-rates-TIF-5ms.txt" title "LIF -60mV (5ms)" with lines lw 3, "60mV-firing-rates-TIF-2ms.txt" title "LIF -60mV (2ms)" with lines lw 3;
plot "58mV-firing-rates.txt" title "AdEx TS -58mV" with lines lw 2, "60mV-firing-rates.txt" title "AdEx TS -60mV" with lines lw 2, "65mV-firing-rates.txt" title "AdEx TS -65mV" with lines lw 2, "70mV-firing-rates.txt" title "AdEx TS -70mV" with lines lw 2, "75mV-firing-rates.txt" title "AdEx TS -75mV" with lines lw 2, "80mV-firing-rates.txt" title "AdEx TS -80mV" with lines lw 2, "85mV-firing-rates.txt" title "AdEx TS -85mV" with lines lw 2, "60mV-firing-rates-TIF-2ms.txt" title "LIF -60mV (2ms)" with lines lw 3;
#plot "60mV-firing-rates.txt" title "AdEx TS -60mV" with lines lw 2, "70mV-firing-rates.txt" title "AdEx TS -70mV" with lines lw 2, "80mV-firing-rates.txt" title "AdEx TS -80mV" with lines lw 2, "60mV-firing-rates-TIF-5ms.txt" title "LIF -60mV (5ms)" with lines lw 3, "60mV-firing-rates-TIF-2ms.txt" title "LIF -60mV (2ms)" with lines lw 3;
