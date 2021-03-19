ALL=power9_dec \
	power9_enc \
	ice_lake_dec \
	ice_lake_enc

all: $(ALL)

power9_dec: power9_dec.txt
	gnuplot -e "filename='power9_dec'" power9.gnuplot

power9_enc: power9_enc.txt
	gnuplot -e "filename='power9_enc'" power9.gnuplot

ice_lake_dec: ice_lake_dec.txt
	gnuplot -e "filename='ice_lake_dec'" ice_lake.gnuplot

ice_lake_enc: ice_lake_enc.txt
	gnuplot -e "filename='ice_lake_enc'" ice_lake.gnuplot

power9_decoding_nanosecondsperinputbyte.png: power9decoding.txt
	gnuplot power9_decoding_nanosecondsperinputbyte.png.gnuplot

clean:
	rm -f *.png
