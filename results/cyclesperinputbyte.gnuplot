# use basic_benchmark to generate skylake.txt

load "linespointsstyle.gnuplot"
set style line 81 lt 0  # dashed
set style line 81 lt rgb "#808080"  # grey
set grid back linestyle 81
set term pdfcairo fontscale 0.75
#set term png fontscale 0.6
set xlabel "input bytes"
set ylabel "CPU cycle per input byte"

stats 'skylake.txt' using 1
set xrange [STATS_min:STATS_max]

set key top right

set out "skylake_cyclesperinputbyte.pdf"

plot "skylake.txt" using 1:2 ti "Linux" w l ls 1, "" using 1:3 ti "Apple QuickTime" w l  ls 2, "" using 1:4 ti "Google Chrome" w l ls 3, "" using 1:5 ti "scalar" w l  ls 4, "" using 1:7 ti "AVX2" w l  ls 6, "" using 1:8 ti "AVX2-Mula" w l  ls 7,  "" using 1:6 ti "AVX2-Kompf" w l  ls 8


set ylabel "CPU cycles / AVX2 cycles"
set out "skylake_ratio_cyclesperinputbyte.pdf"

plot "skylake.txt" using 1:($4/$7) notitle w l ls 8



set term pngcairo fontscale 1.5 size 1024,768
set out "skylake_cyclesperinputbyte.png"

set ylabel "CPU cycle per input byte"



plot "skylake.txt" using 1:2 ti "Linux" w l ls 1, "" using 1:3 ti "Apple QuickTime" w l  ls 2, "" using 1:4 ti "Google Chrome" w l ls 3, "" using 1:5 ti "scalar" w l  ls 4,"" using 1:7 ti "AVX2" w l  ls 6, "" using 1:8 ti "AVX2-Mula" w l  ls 7,  "" using 1:6 ti "AVX2-Kompf" w l  ls 8
