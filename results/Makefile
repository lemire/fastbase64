ALL=skylake_decoding_cyclesperinputbyte.pdf \
    skylake_decoding_cyclesperinputbyte.png \
    skylake_decoding_ratio_cyclesperinputbyte.pdf \
    skylake_encoding_cyclesperinputbyte.pdf \
    skylake_encoding_cyclesperinputbyte.png \
    skylake_encoding_ratio_cyclesperinputbyte.pdf

all: $(ALL)

skylake_decoding_cyclesperinputbyte.pdf: skylake.txt 
	gnuplot skylake_decoding_cyclesperinputbyte_pdf.gnuplot

skylake_decoding_ratio_cyclesperinputbyte.pdf: skylake.txt
	gnuplot skylake_decoding_ratio_cyclesperinputbyte.gnuplot

skylake_decoding_cyclesperinputbyte.png: skylake.txt
	gnuplot skylake_decoding_cyclesperinputbyte_png.gnuplot

skylake_encoding_cyclesperinputbyte.pdf: skylakeencoding.txt
	gnuplot skylake_encoding_cyclesperinputbyte_pdf.gnuplot

skylake_encoding_cyclesperinputbyte.png: skylakeencoding.txt
	gnuplot skylake_encoding_cyclesperinputbyte_png.gnuplot

skylake_encoding_ratio_cyclesperinputbyte.pdf: skylakeencoding.txt
	gnuplot skylake_encoding_ratio_cyclesperinputbyte.gnuplot

clean:
	rm -f *.pdf *.png
