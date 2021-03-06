/**************************************************************************
 * Copyright (C) 2014 Riccardo Binetti, Guido Gerosa, Alessandro Mariani  *
 * Copyright (C) 2017 Giovanni Beri                                       *
 *                                                                        *
 * This program is free software: you can redistribute it and/or modify   *
 * it under the terms of the GNU General Public License as published by   *
 * the Free Software Foundation, either version 3 of the License, or      *
 * (at your option) any later version.                                    *
 *                                                                        *
 * This program is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.  *
 **************************************************************************/

/* 
 * File:   Microphone.h
 * Author: grp51
 *
 * Created on December 29, 2013, 4:54 PM
 * 
 * This class defines a simple interface for recording audio with the embedded
 * microphone on the STM32F4 Discovery board and compressing it using an ADPCM encoder
 * 
 * It works by specifying which user-defined function should process the PCM samples
 * and how many of them. Then the driver can start recordin and the defined function
 * will be called repeatedly each time the specified number of PCM samples have 
 * been produced.
 * 
 * Internally, it takes care of copying the microphone output in RAM via DMA and
 * transcoding such output from PDM to PCM. The latter is done via CIC filtering.
 * 
 */


#include "miosix.h"
#include "codec.h"
#include <functional>

#ifndef MICROPHONE_H
#define	MICROPHONE_H


/*
 * The Microphone class is the singleton that handles everything: the device 
 * configuration, the recording and the transcoding from PDM to PCM 
 */
class Microphone {
public:
    
    /* 
     * \return the instance of the Microphone class to be used for recording.
     */
    static Microphone& instance();
    
    
    /*
     * Initialize the driver for recording and handling of the audio
     * 
     * \param cback the callback function that will be executed (repeatedly) when
     * the driver produces the right number of samples
     */
    void init(std::function<void (unsigned char*, int)> cback);
    
    /*
     * Starts the recording. When start() is called the devices configuration
     * registers are set and the DMA starts copying the microphone PDM samples in RAM.
     * The call is non-blocking, the processing from PDM to PCM and the callbacks
     * are executed in threads.
     */
    void start();
    
    /*
     * Wait for the last chunk of PCM samples to be processed, stop the DMA and 
     * reset the configuration registers.
     */
    void stop();
    
    /*
     * Return the size of a batch
     */
     int getBatchSize();

private:
    Microphone(); // Microphone is a singleton, the constructor is private
    Microphone(const Microphone& orig);
    virtual ~Microphone();
    std::function<void (unsigned char*, int)> callback;
    // the buffers handling the double buffering "callback-side"
    short* readyBuffer;
    short* processingBuffer;
    // variables used to track  and store the transcoding progess
    unsigned int PCMsize;
    unsigned int PCMindex;
    
    volatile bool recording;    
    pthread_t mainLoopThread;
    bool processPDM(const unsigned short *pdmbuffer, int size);
    unsigned short PDMFilter(const unsigned short* PDMBuffer, unsigned int index);
    void mainLoop();
    void execCallback();
    static void* callbackLauncher(void* arg);
    static void* mainLoopLauncher(void* arg);
    
    volatile bool isBufferReady;
    pthread_mutex_t bufMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cbackExecCond = PTHREAD_COND_INITIALIZER; 

    // buffers used to perorm decimation 
    short int* decimatedReadyBuffer;
    short int* decimatedProcessingBuffer;
    // adpcm encoder data
    unsigned char* compressedBuf;
    CodecState state;
    int compressed_buf_size_bytes;

};

#endif	/* MICROPHONE_H */

