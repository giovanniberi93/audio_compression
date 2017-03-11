#!/usr/bin/python3
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
# if 	ser.isOpen()
# 	print("Error opening serial port")
# 	print(usage)
# 	exit( 1 )
outputFile = open(sys.argv[3],"wb");

#size of the sample 
bytes_per_int = 2

pattern = re.compile("(-)?\d+");
sample = ""
while sample != beginSignal: #wait for the begin signal
	sample = ser.readline().decode();
	sample = sample.split("\r\n")[0]
first = True
while True:
	sample = ser.readline().decode()
	# print("1: sample is $"+ sample)
	while sample != beginSignal :
		if first == True:
			first = False
			sample = sample.split("\r\n")[0]
		else :
			sample = ser.readline().decode().split("\r\n")[0]
		match = pattern.match(sample) 
		if match != None:
			# here all and only the samples are processed
			sample = match.group()
			intSample = int(sample)
			print("sample is $"+ sample)
			outputFile.write(intSample.to_bytes(bytes_per_int, 'little', signed=True))
		else :
			first = True
			# print "# Regular expression error: ",sample 
