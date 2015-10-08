set term postscript   color enhanced eps 24 dashlength 2 rounded
set size 1,0.8
LW = 4
PS = 2
set xrange[0:80]
set yrange[0:110]
set style line 1 lt rgb "#808080"  # grey
set style line 2 lt rgb "#000000"  # black
set ylabel "Prop. received packets (%)"
set xlabel "Time (minutes)"
set key outside 
set key over
plot  FILE using ($1)-0.75:($2):($2-$3):($2+$3) w yerrorbars ls 1 lw LW ps PS title "Multichannel", \
     FILE using ($1)+0.75:($4):($4-$5):($4+$5) w yerrorbars ls 2 lw LW ps PS title "Single channel"
