#!/usr/bin/python3
#
# This script plots vectors received via serial as unsigned short integer. Each vector is
# separated by the string "new"
#
# Usage: desktop_script.py device baudrate filename
#
# Example: ./desktop_script.py /dev/ttyUSB0 19200 myFile
#

import sys, serial, re
import ctypes

def int16_to_uint16(i):
    return ctypes.c_uint16(i).value


beginSignal = "new"
usage = "Usage: " + str(sys.argv[0]) +" device baudrate myFile.raw" + "\n\nexample:\n"+ sys.argv[0] + " /dev/ttyUSB0 19200 myFile.raw";

if len(sys.argv) < 4:
	print("Too few arguments")
	print(usage)
	exit( 1 )
ser = serial.Serial(str(sys.argv[1]), int(sys.argv[2])) #no timeout specified

outputFile = open(sys.argv[3],"wb");

#size of the sample 
bytes_per_int = 2

pattern = re.compile("(-)?\d+");
sample = ""
while sample != beginSignal: #wait for the begin signal
	sample = ser.readline().decode()
	sample = sample.split("\r\n")[0]
first = True
while True:
	sample = ser.readline().decode()
	while sample != beginSignal :
		if first == True :
			first = False
			sample = sample.split("\r\n")[0]
		else :
			sample = ser.readline().decode().split("\r\n")[0]
		match = pattern.match(sample) 
		if match != None:
			# here all and only the samples are processed
			sample = match.group()
			intSample = int(sample)
			adjustedIntSample = int16_to_uint16(intSample)

			print("sample is $", intSample, "adjusted to ",adjustedIntSample)
			outputFile.write(adjustedIntSample.to_bytes(bytes_per_int, 'little', signed=False))
		else :
			first = True
