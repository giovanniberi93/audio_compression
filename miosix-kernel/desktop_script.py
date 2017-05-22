#!/usr/bin/python3 
#
# Usage: desktop_script.py filename
# Example: ./desktop_script.py out.wav
#
# IMPORTANT: this script relies on executable ./adpcm/decoder. Follow instruction 
# 			in readme file in order to create it, if you have not done it yet 

import sys, serial
from subprocess import call




beginSignal = "ready"

if len(sys.argv) == 1 :
	fileName = "out.wav"
	print("Default name 'out.wav' assigned. Use: \n\t ./desktop_script myFileName.wav \nto specify a different name.")
else :
	fileName = sys.argv[1]

# setup serial port
# no timeout specified
ser = serial.Serial("/dev/ttyUSB0",					\
					57600,							\
					stopbits = serial.STOPBITS_ONE,	\
					parity = serial.PARITY_NONE,	\
					bytesize = serial.EIGHTBITS,	\
					timeout = None, 				\
					rtscts = False,					\
					dsrdtr = False,					\
					xonxoff = False
					)

outputFile = open(".tmp","wb")
# outputFile = open(sys.argv[3],"wb")

# wait begin signal
print("\nWaiting for the recording to start...")
sample = ""
while sample != beginSignal:
	sample = ser.readline().decode()
	sample = sample.split("\n")[0]
print("\t. : packet received correctly")
print("\t# : end of the recording")
print("Started recording", end='', flush=True)

nextBatchSizeInt = -1;	
while nextBatchSizeInt != 0:
	# read size of next batch
	nextBatchSize = ser.read(4)
	nextBatchSizeInt = int.from_bytes(nextBatchSize,byteorder='little', signed=True)
	print('.', end='', flush=True)
	# reads the batch
	sample = ser.read(size=nextBatchSizeInt)
	# write data to file
	outputFile.write(sample)
print("#\n")
print("Recording completed with success!")
# decode adpcm to wav file
successfulDecompression = call(["adpcm/decoder", ".tmp", fileName], stdout=None)
if successfulDecompression < 0 :
	print("Error in decompression phase")
else :
	# and remove temporary adpcm file
	print("Decompression completed with success!")
	call(["rm",".tmp"])
	print("\nPlay with:\n\n    ffplay -ar 11025 " + fileName)

