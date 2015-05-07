First use python to process the files that come from the excel.

./filterdata.py extreme.dat > extreme.proc
./filterdata.py moderate.dat > moderate.proc
./filterdata.py mild.dat > mild.proc

gnuplot -e "FILE='mild.proc'" plot_interfere.gnu > mild.eps
epstopdf mild.eps
gnuplot -e "FILE='moderate.proc'" plot_interfere.gnu > moderate.eps
epstopdf moderate.eps
gnuplot -e "FILE='extreme.proc'" plot_interfere.gnu > extreme.eps
epstopdf extreme.eps
