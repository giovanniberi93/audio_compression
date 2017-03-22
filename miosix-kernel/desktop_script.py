#!/usr/bin/python3
#
# This script plots vectors received via serial as unsigned short integer. Each vector is
# separated by the string "new"
#
# Usage: desktop_script.py device baudrate filename
#
# Example: ./desktop_script.py /dev/ttyUSB0 115200 myFile
#

import sys, serial, re

beginSignal = "new"
usage = "Usage: " + str(sys.argv[0]) +" device baudrate myFile.raw" + "\n\nexample:\n"+ sys.argv[0] + " /dev/ttyUSB0 115200 myFile.raw";

if len(sys.argv) < 4:
	print("Too few arguments")
	print(usage)
	exit( 1 )

# setup serial port
ser = serial.Serial(str(sys.argv[1]), int(sys.argv[2]),stopbits=serial.STOPBITS_ONE,parity=serial.PARITY_NONE, bytesize=serial.EIGHTBITS) #no timeout specified

outputFile = open(sys.argv[3],"wb");
outputFileTxt = open(sys.argv[3] + ".txt", "w")

print("prima del begin signal")
sample = ""
while sample != beginSignal: #wait for the begin signal
	sample = ser.readline().decode()
	sample = sample.split("\n")[0]
print("dopo il begin signal")
# read the batch size
size = ser.readline().decode();
# because every sample is made of 2 bytes
bytesPerBatch = int(size)*2
print("bytes per batch: ",bytesPerBatch)

while True:
	sample = ser.read(size=bytesPerBatch)
	intSample = int.from_bytes(sample,byteorder='little', signed=False)
	print("sample is ",str(intSample))
	# samples are in little endian
	outputFile.write(sample)
	outputFileTxt.write(str(intSample)+"\n")
