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
singlemean= []
multmean= []
singlesd= []
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
        if len(n) != 21:
            print >> sys.stderr, "Expects 21 numbers on a line"
        times.append(float(n[0]))
        sing=[]
        mult=[]
        for i in range(10):
            mult.append(float(n[1+(i*2)+1]))
            sing.append(float(n[1+(i*2)]))
        singlemean.append(numpy.mean(sing))
        multmean.append(numpy.mean(mult))
        singlesd.append(numpy.std(sing))
        multsd.append(numpy.std(mult))
    for i in range(len(times)):
        print >>  sys.stdout, times[i],singlemean[i],singlesd[i],multmean[i],multsd[i]
    f.close()
    
except Exception, err:
    print traceback.format_exc()
    print sys.stderr, "Parsing/read error in ",fileName
    sys.exit()
