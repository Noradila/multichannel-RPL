#!/usr/bin/env python

import numpy
import getopt
import sys
import traceback

def printUsage():
    print >> sys.stderr, \
        './filterdata.py [inputfile.dat] > outputfile.dat'
    print >> sys.stderr, "Produces mean and stddev for each time slice"
    sys.exit()

try:
    (opts, args) = getopt.getopt(sys.argv[1:], 'h')
except getopt.GetoptError, err:
    print >> sys.stderr, 'Unrecognised option', err
    printUsage()
for (o,a) in opts:
    if o == '-h':
        printUsage()

if len(args) != 1:
    print >> sys.stderr
    printUsage()
fileName=args[0]


times= []
multmean= []
multsd= []
try: 
    f= open(fileName,'r')
    while True:
        l= f.readline()
        if l == '':
            break
        n= l.split()
        if len(n) == 0:
            continue
        if len(n) != 11:
            print >> sys.stderr, "Expects 11 numbers on a line"
        times.append(float(n[0]))
        sing=[]
        mult=[]
        for i in range(10):
            mult.append(float(n[i+1]))
        multmean.append(numpy.mean(mult))
        multsd.append(numpy.std(mult))
    for i in range(len(times)):
        print >>  sys.stdout, times[i],multmean[i],multsd[i]
    f.close()
    
except Exception, err:
    print traceback.format_exc()
    print sys.stderr, "Parsing/read error in ",fileName
    sys.exit()
