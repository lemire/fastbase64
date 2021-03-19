load "linespointsstyle.gnuplot"

# filename is "ice_lake_dec" or "ice_lake_enc"

stats filename.".txt" using 1

set term pngcairo fontscale 1.5 size 1024,768
set style line 81 lt 0  # dashed
set style line 81 lt rgb "#808080"  # grey
set grid back linestyle 81
set xrange [STATS_min:STATS_max]
set ytics 0.5
set key top right box opaque
set title "Ice Lake"
set xlabel "input bytes"

set ylabel "CPU cycles per input byte"
set yrange [0:5]
set out filename.".png"
plot filename.".txt" \
        using 1:3 ti "Linux"            smooth csplines ls 1, \
     "" using 1:4 ti "Apple QuickTime"  smooth csplines ls 2, \
     "" using 1:5 ti "Google Chrome"    smooth csplines ls 3, \
     "" using 1:6 ti "scalar"           smooth csplines ls 4, \
     "" using 1:7 ti "AVX2 (Klomp)"     smooth csplines ls 5, \
     "" using 1:8 ti "AVX2"             smooth csplines ls 6, \
     "" using 1:9 ti "AVX512BW"         smooth csplines ls 7, \
     "" using 1:2 ti "memcpy"           smooth csplines ls 8

set ylabel "CPU cycles / AVX2 cycles"
set yrange [0:10]
set out filename."_ratio.png"
plot filename.".txt" using 1:($4/$7) notitle w l ls 8