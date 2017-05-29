
# Welcome to the Miosix kernel


You can find information on how to configure and use the kernel
at the following url: http://miosix.org

============================
 
## OVERVIEW

This application is designed for Miosix 2.02 on the STM32F407 Discovery board. It uses the on-board MEMS microphone to collect audio samples, which are compressed via ADPCM and sent on the serial port. A desktop python script takes care of reading the compressed samples and writing in a temporary file. When the "end of recording" signal occurs, it decompresses the audio and creates an output .wav file.


## YOU NEED (Ubuntu 16.04 assumed; no other environment have been tested)

 - python3: it should already been installed, test with `$ python3 -V`
 - pyserial: `$ sudo apt-get install python3-serial`
 - a .wav player that let you set the output sample rate; I've used ffplay and I liked it: 
 `$ sudo apt-get install ffmpeg`
 - an RS-232 cable like this one (https://goo.gl/CFUBLj)


## BEFORE STARTING

 You must build the executable that desktop_script will call to decode collected audio samples. In order to do this, open a terminal in 'miosix-kernel/adpcm' directory and type (ignore warnings):
 
 	$ g++ mainDecoder.cpp codec.cpp -o decoder 
 To test it, try:
 
 	$ ./decoder decodeTest test.wav
 	$ ffplay -ar 11025 test.wav
    
 If it works, remove the test .wav file with
 
    $ rm test.wav
and, if you want, also the test adpcm file with
 	
    $ rm decodeTest

 After that, you only need to compile the firmware, with the usual command `$ make` in 'miosix-kernel' directory.


## HOW TO USE IT

 Because of some conflicts in the DMA channels, this firmware uses USART2 as default, so you need to connect RS-232 pins to PA2 and PA3. Once it's done, the usage is simple:
 - launch desktop_script.py
 - press user button to start recording, and press it again to stop it
 
 It could be possible that:
 - communication errors due to RS-232 limits corrupt some data on the link, so the script may not be able to collect data coming from the board. You can identify this situation because every time the script collect a batch of data, it printf a dot on the screen. If the red led is on in the board but dots stop appearing, you have to reset the board and the script
 - the user button takes 2 clicks at time, so it stops immediately the recording. If it happens, just make another try. 

 However, some measures has been taken to solve these situations, so they should not happen often.

 Just in case you are struggling because you are stuck using or understanding some part of this code, feel free to contact me on GitHub :whale: