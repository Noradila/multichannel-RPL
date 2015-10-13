set term postscript   color enhanced eps 24 dashlength 2 rounded
set size 1,0.8
LW = 4
PS = 2
set xrange[28:55]
set yrange[94:100.9]
set style line 1 lt rgb "#808080"  # grey
set style line 2 lt rgb "#000000"  # black
set style line 3 lt rgb "#101010"  # dark grey
set ylabel "Prop. received packets (%)"
set xlabel "Time (minutes)"
set key outside 
set key over
plot "multi_channel_scenario1.proc" using ($1)-0.75:($2):($2-$3):($2+$3) w yerrorbars ls 1 lw LW ps PS title "Scenario 1", \
    "multi_channel_scenario2.proc" using ($1)+0.75:($2):($2-$3):($2+$3) w yerrorbars ls 2 lw LW ps PS title "Scenario 2"
