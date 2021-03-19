set term pngcairo fontscale 1.5 size 1024,768
set out filename.".png"
load "linespointsstyle.gnuplot"
set style line 81 lt 0  # dashed
set style line 81 lt rgb "#808080"  # grey
set grid back linestyle 81
set xlabel "input bytes"
set ylabel "nanoseconds per input byte"

stats filename.".txt" using 1
set xrange [STATS_min:STATS_max]
set ytics 0.5
set yrange [0:5]
set key top right box opaque
set title "power9"
plot filename.".txt" \
        using 1:3 ti "Linux"            smooth csplines ls 1, \
     "" using 1:4 ti "Apple QuickTime"  smooth csplines ls 2, \
     "" using 1:5 ti "Google Chrome"    smooth csplines ls 3, \
     "" using 1:6 ti "scalar"           smooth csplines ls 4, \
     "" using 1:7 ti "AltiVec"          smooth csplines ls 5, \
     "" using 1:2 ti "memcpy"           smooth csplines ls 6, \
