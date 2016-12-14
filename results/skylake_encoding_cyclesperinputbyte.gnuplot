load "linespointsstyle.gnuplot"
set style line 81 lt 0  # dashed
set style line 81 lt rgb "#808080"  # grey
set grid back linestyle 81
set xlabel "input bytes"
set ylabel "CPU cycles per input byte"

stats 'skylakeencoding.txt' using 1
set xrange [STATS_min:STATS_max]
set ytics 0.5
set yrange [0:5]
set key top right

plot "skylakeencoding.txt" using 1:2 ti "Linux"  smooth csplines ls 1, \
     "" using 1:3 ti "Apple QuickTime"  smooth csplines  ls 2, \
     "" using 1:4 ti "Google Chrome"  smooth csplines ls 3, \
     "" using 1:6 ti "AVX2"  smooth csplines  ls 6
