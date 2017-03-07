#!/usr/bin/python
#
# This script plots vectors received via serial as couples of 
# indexes and values saparated by a semicolon. Each vector is
# separated by the string "new"
#
# Usage: plot.py device baudrate filename
#
# Esample: ./plot.py /dev/ttyUSB0 19200
#

import sys, serial, re

beginSignal = "new"
usage = """Usage: plot.py device baudrate

plot.py /dev/ttyUSB0 19200 myFile""";

if len(sys.argv) < 4:
	print("Too few arguments")
	print(usage)
	exit( 1 )
ser = serial.Serial(str(sys.argv[1]), int(sys.argv[2])) #no timeout specified
if ser < 1:
	print("Error opening serial port")
	print(usage)
	exit( 1 )
outputFile = open(sys.argv[3],"a");

# pattern = re.compile("(-)?\d+(.\d+)?:(-)?\d+(.\d+)?");
pattern = re.compile("(-)?\d+(.\d+)?");
sample = ""
while sample != beginSignal: #wait for the begin signal
	sample = ser.readline().split("\r\n")[0]
print(sample)
while True:
	sample = ser.readline()
	while sample != beginSignal:
		sample = ser.readline().split("\r\n")[0]
		# print("sample is $", sample)
		match = pattern.match(sample) 
		if match != None:
			sample = match.group()
			outputFile.write(sample+" ")
		else :
			print "# Regular expression error: ",sample 
