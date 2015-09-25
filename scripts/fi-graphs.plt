set terminal pngcairo size 1536,768;
set output "AdEx-ab0-FI.png";
#set terminal epslatex size 5,2.5 color colortext
#set output "AdEx-ab0-FI.tex";

set border 3;
set xlabel "Input current (pA)";
set ylabel "Firing rate in (Hz)";
set xrange [0:];
set xtics nomirror 0,2000
set ytics nomirror
set key left
#set title "F-I curve for AdEx with ab=0 and different reset voltages"
plot "58mV-firing-rates.txt" title "-58mV" with lines lw 2, "60mV-firing-rates.txt" title "-60mV" with lines lw 2, "65mV-firing-rates.txt" title "-65mV" with lines lw 2, "70mV-firing-rates.txt" title "-70mV" with lines lw 2, "75mV-firing-rates.txt" title "-75mV" with lines lw 2, "80mV-firing-rates.txt" title "-80mV" with lines lw 2, "85mV-firing-rates.txt" title "-85mV" with lines lw 2;
