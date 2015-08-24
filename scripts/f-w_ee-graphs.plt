set terminal pngcairo size 1536,768;
set output "AdEx-ab0-F-w_ee.png";
#set terminal epslatex size 5,2.5 color colortext
#set output "AdEx-ab0-F-w_ee.tex";

set border 3;
set xlabel 'k\_w\_ee (w\_ee = k\_w\_ee * 3nS)';
set ylabel "Firing rate in (Hz)";
#set xrange [0:2100];
#set xtics nomirror 0,200,2500
set xtics nomirror
set ytics nomirror
set key left
set title 'F vs k\_w\_ee for 8000 e neurons, 2000 i neurons, sparsity of 0.02 - no IE - 220pA bg current'
#plot "58mV-firing-rates.txt" title "-58mV" with lines lw 2, "60mV-firing-rates.txt" title "-60mV" with lines lw 2, "65mV-firing-rates.txt" title "-65mV" with lines lw 2, "70mV-firing-rates.txt" title "-70mV" with lines lw 2, "75mV-firing-rates.txt" title "-75mV" with lines lw 2, "80mV-firing-rates.txt" title "-80mV" with lines lw 2, "85mV-firing-rates.txt" title "-85mV" with lines lw 2;
plot "firing-rates-e.txt" title "E neurons" with lines lw 2, "firing-rates-i.txt" title "I neurons" with lines lw 2;

set output "AdEx-ab0-F-w_ee-e.png";
#set terminal epslatex size 5,2.5 color colortext
#set output "AdEx-ab0-F-w_ee.tex";

set border 3;
set xlabel 'k\_w\_ee (w\_ee = k\_w\_ee * 3nS)';
set ylabel "Firing rate in (Hz)";
#set xrange [0:2100];
#set xtics nomirror 0,200,2500
set xtics nomirror
set ytics nomirror
set key left
set title 'F vs k\_w\_ee for 8000 e neurons sparsity of 0.02 - no IE - 220pA bg current'
#plot "58mV-firing-rates.txt" title "-58mV" with lines lw 2, "60mV-firing-rates.txt" title "-60mV" with lines lw 2, "65mV-firing-rates.txt" title "-65mV" with lines lw 2, "70mV-firing-rates.txt" title "-70mV" with lines lw 2, "75mV-firing-rates.txt" title "-75mV" with lines lw 2, "80mV-firing-rates.txt" title "-80mV" with lines lw 2, "85mV-firing-rates.txt" title "-85mV" with lines lw 2;
plot "firing-rates-e.txt" title "E neurons" with lines lw 2;

set output "AdEx-ab0-F-w_ee-i.png";
#set terminal epslatex size 5,2.5 color colortext
#set output "AdEx-ab0-F-w_ee.tex";

set border 3;
set xlabel 'k\_w\_ee (w\_ee = k\_w\_ee * 3nS)';
set ylabel "Firing rate in (Hz)";
#set xrange [0:2100];
#set xtics nomirror 0,200,2500
set xtics nomirror
set ytics nomirror
set key left
set title 'F vs k\_w\_ee for 2000 i neurons sparsity of 0.02 - no IE - 220pA bg current'
#plot "58mV-firing-rates.txt" title "-58mV" with lines lw 2, "60mV-firing-rates.txt" title "-60mV" with lines lw 2, "65mV-firing-rates.txt" title "-65mV" with lines lw 2, "70mV-firing-rates.txt" title "-70mV" with lines lw 2, "75mV-firing-rates.txt" title "-75mV" with lines lw 2, "80mV-firing-rates.txt" title "-80mV" with lines lw 2, "85mV-firing-rates.txt" title "-85mV" with lines lw 2;
plot "firing-rates-i.txt" title "I neurons" with lines lw 2;
