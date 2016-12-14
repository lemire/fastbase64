load "linespointsstyle.gnuplot"
set style line 81 lt 0  # dashed
set style line 81 lt rgb "#808080"  # grey
set grid back linestyle 81
set xlabel "input bytes"
set ylabel "CPU cycles per input byte"

stats 'skylake.txt' using 1
set xrange [STATS_min:STATS_max]

set key top right

plot "skylake.txt" using 1:2 ti "Linux" w l ls 1, \
     "" using 1:3 ti "Apple QuickTime" w l  ls 2, \
     "" using 1:4 ti "Google Chrome" w l ls 3, \
     "" using 1:7 ti "AVX2" w l  ls 6
