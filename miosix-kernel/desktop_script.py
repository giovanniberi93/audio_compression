#!/usr/bin/python3 
#
# Copyright (C) 2017 Giovanni Beri
#
# Usage: desktop_script.py filename
# Example: ./desktop_script.py out.wav
#
# IMPORTANT: this script relies on executable ./adpcm/decoder. Follow instruction 
# 			in readme file in order to create it, if you have not done it yet 

import sys, serial
from subprocess import call

# bytes representing the expected size of the audio data chunk
# what it does is read bytes until it reach the head of a new chunk of data
# i.e. the size of the chunk (that should be the expected batch size)
def recoveryProcedure(expectedBatchSize):
	""" tries to perform recovery in case of a communication error """
	s = ser.read(size=1)
	while True :
		if s != expectedBatchSize[0:1]:
			s = ser.read(size=1)
		else :
			s = ser.read(size=1)
			if s == expectedBatchSize[1:2] :
				s = ser.read(size=1)
				if s == expectedBatchSize[2:3]:
					s = ser.read(size=1)
					if s == expectedBatchSize[3:4]:
						return int.from_bytes(expectedBatchSize,byteorder='little', signed=True)



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
print("\t! : corrupted packet fixed")
print("\t# : end of the recording")
print("Started recording", end='', flush=True)

# get expected batch size, as a byte string
expectedBatchSize = ser.read(4)
expectedBatchSizeInt = int.from_bytes(expectedBatchSize,byteorder='little', signed=True)
nextBatchSizeInt = -1;	

while nextBatchSizeInt != 0:
	# read size of next batch
	nextBatchSize = ser.read(4)
	nextBatchSizeInt = int.from_bytes(nextBatchSize,byteorder='little', signed=True)
	# it means that an error occured
	if nextBatchSizeInt != expectedBatchSizeInt and nextBatchSizeInt != 0 :
		nextBatchSizeInt = recoveryProcedure(expectedBatchSize)
		print('!', end='', flush=True)
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

