for X in 1 2 3 4 5 
do
    ./basic_benchmark
    cp decodingperf.txt decodingperf$X.txt
    cp encodingperf.txt encodingperf$X.txt
done
