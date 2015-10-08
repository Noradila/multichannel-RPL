###  Processing for first submitted paper graphs

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


#### Processing for second submitted paper graphs

As before use python to process the files that come from the excel.

./filterdata.py extreme.dat > extreme.proc
./filterdata.py moderate.dat > moderate.proc
./filterdata.py mild.dat > mild.proc

gnuplot single_channel.gnu > single_channel.eps
epstopdf single_channel.pdf

./filterdata2.py multi_channel_scenario1.dat > multi_channel_scenario1.proc
./filterdata2.py multi_channel_scenario2.dat > multi_channel_scenario2.proc

gnuplot multi_channel.gnu > multi_channel.eps
epstopdf multi_channel.eps
