#!/usr/bin/python
#
# This script plots vectors received via serial as couples of 
# indexes and values saparated by a semicolon. Each vector is
# separated by the string "new"
#
# Usage: plot.py device baudrate filename
#
# Esample: ./plot.py /dev/ttyUSB0 19200 myFile
#

import sys, serial, re

beginSignal = "new"
usage = "Usage: " + str(sys.argv[0]) +" device baudrate myFile" + "\n\nexample:\n"+ sys.argv[0] + " /dev/ttyUSB0 19200 myFile";

if len(sys.argv) < 4:
	print("Too few arguments")
	print(usage)
	exit( 1 )
ser = serial.Serial(str(sys.argv[1]), int(sys.argv[2])) #no timeout specified
if ser < 1:
	print("Error opening serial port")
	print(usage)
	exit( 1 )
outputFile = open(sys.argv[3],"w");

# pattern = re.compile("(-)?\d+(.\d+)?:(-)?\d+(.\d+)?");
pattern = re.compile("(-)?\d+");
sample = ""
while sample != beginSignal: #wait for the begin signal
	sample = ser.readline().split("\r\n")[0]
first = True
while True:
	sample = ser.readline()
	# print("1: sample is $"+ sample)
	while sample != beginSignal :
		if first == True:
			first = False
			sample = sample.split("\r\n")[0]
		else :
			sample = ser.readline().split("\r\n")[0]
		match = pattern.match(sample) 
		if match != None:
			# here all and only samples different from beginSignal are processed
			sample = match.group()
			print("sample is $"+ sample)
			outputFile.write(sample+" ")
		else :
			first = True
			# print "# Regular expression error: ",sample 
