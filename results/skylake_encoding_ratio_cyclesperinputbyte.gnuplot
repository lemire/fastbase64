load "linespointsstyle.gnuplot"

set style line 81 lt 0  # dashed
set style line 81 lt rgb "#808080"  # grey
set grid back linestyle 81
set xlabel "input bytes"
set ylabel "CPU cycles / AVX2 cycles"

set term pdfcairo fontscale 0.75
set out "skylake_encoding_ratio_cyclesperinputbyte.pdf"

plot "skylake.txt" using 1:($4/$6) notitle w l ls 8
