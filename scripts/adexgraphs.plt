# To generate the adexgraphs
# Shell command something like this:
# LD_LIBRARY_PATH=/home/asinha/Documents/02_Code/00_repos/00_mine/herts-research-repo/auryn/src/.libs ~/bin/research-bin/adextest ; head -n 2000 voltages.txt > voltages-2000.txt; gnuplot ../../../../src/postprocess/scripts/adexgraphs.plt ; rm voltages.txt output.log -f; rename "adex" "4d" * ; sed -i "s/adex/4d/" 4d.tex; git add . ;

set terminal epslatex size 2.5,2.5 color colortext
set output "adex.tex";

#set border 3;
unset border;
unset xtics
unset ytics
#set xtics nomirror ('\(0\)' 0, '\(200\)' 0.2);
#set ytics nomirror ('\(-70\)' -0.07, '\(0\)' 0, '\(20\)' 0.02);
set yrange [-0.07:0.03]
#set xrange [0:0.20]
set lmargin 3
set bmargin 2
# horizontal
set arrow from graph -0.05,-0.05 to graph 0.05,-0.05 nohead lw 2
# vertical
set arrow from graph -0.05,-0.05 to graph -0.05,0.250 nohead lw 2


#set label '\(20ms\)' at graph -0.05,-0.1 
set label '\(30mV\)' at graph -0.1,-0.05 rotate left

#plot "voltages-2000.txt" title "" with lines lc 1 lw 2;

# changes for 4f
# LD_LIBRARY_PATH=/home/asinha/Documents/02_Code/00_repos/00_mine/herts-research-repo/auryn/src/.libs ~/bin/research-bin/adextest ; head -n 5000 voltages.txt > voltages-5000.txt; gnuplot ../../../../src/postprocess/scripts/adexgraphs.plt ; rm voltages.txt output.log -f; rename "adex" "4f" * ; sed -i "s/adex/4f/" 4f.tex; git add . ;
set xrange [0:0.50]
set label '\(50ms\)' at graph -0.05,-0.1 
plot "voltages-5000.txt" title "" with lines lc 1 lw 2;
