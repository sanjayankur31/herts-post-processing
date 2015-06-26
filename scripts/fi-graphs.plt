set terminal pngcairo size 1536,768;
set output "AdEx-ab0-FI.png";

set xlabel "Current (pA)";
set ylabel "Firing rate in (Hz)";
set xrange [0:2100];
set xtics 0,200,2200
set title "F-I curve for AdEx with ab=0 and different reset voltages"
plot "58mV-firing-rates.txt" title "-58mV" with lines lw 2, "60mV-firing-rates.txt" title "-60mV" with lines lw 2, "65mV-firing-rates.txt" title "-65mV" with lines lw 2, "70mV-firing-rates.txt" title "-70mV" with lines lw 2, "75mV-firing-rates.txt" title "-75mV" with lines;
