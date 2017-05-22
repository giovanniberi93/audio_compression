/**************************************************************************
 * Copyright (C) 2014 Riccardo Binetti, Guido Gerosa, Alessandro Mariani  *
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

#include <cstdio>
#include "miosix.h"
#include "miosix/kernel/queue.h"
#include "miosix/kernel/scheduler/scheduler.h"
#include "util/software_i2c.h"
#include "Microphone.h"
#include "codec.h"

// it keeps only 1/DECIMATION_FACTOR of audio samples
#define DECIMATION_FACTOR 4
// number of samples to be processed at time
#define SAMPLES_AT_TIME 5000


using namespace std;
using namespace miosix;

typedef Gpio<GPIOB_BASE,10> clk;
typedef Gpio<GPIOC_BASE,3> dout;



static const int bufferSize=512; //Buffer RAM is 4*bufferSize bytes
static const int bufNum = 2;
static Thread *waiting;
static BufferQueue<unsigned short,bufferSize,bufNum> *bq;
static bool enobuf=true;
static const char filterOrder = 4;
static const short oversample = 16;
static unsigned short intReg[filterOrder] = {0,0,0,0};
static unsigned short combReg[filterOrder] = {0,0,0,0};
static signed char pdmLUT[] = {-1, 1};

/**
 * Configure the DMA to do another transfer
 */
static void IRQdmaRefill()
{
    unsigned short *buffer;
    
    if(bq->tryGetWritableBuffer(buffer)==false)
    {
        enobuf=true;
        return;
    }
    DMA1_Stream3->CR=0;
    DMA1_Stream3->PAR=reinterpret_cast<unsigned int>(&SPI2->DR);
    DMA1_Stream3->M0AR=reinterpret_cast<unsigned int>(buffer);
    DMA1_Stream3->NDTR=bufferSize;
    DMA1_Stream3->CR=DMA_SxCR_PL_1    | //High priority DMA stream
                     DMA_SxCR_MSIZE_0 | //Write 16bit at a time to RAM
                     DMA_SxCR_PSIZE_0 | //Read 16bit at a time from SPI
                     DMA_SxCR_MINC    | //Increment RAM pointer
                     DMA_SxCR_TCIE    | //Interrupt on completion
                     DMA_SxCR_EN;       //Start the DMA
}




static void dmaRefill()
{
    FastInterruptDisableLock dLock;
    IRQdmaRefill();
}

/**
 * DMA end of transfer interrupt
 */
void __attribute__((naked)) DMA1_Stream3_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z17I2SdmaHandlerImplv");
    restoreContext();
}

/**
 * DMA end of transfer interrupt actual implementation
 */
void __attribute__((used)) I2SdmaHandlerImpl()
{
    
    DMA1->LIFCR=DMA_LIFCR_CTCIF3  |
                DMA_LIFCR_CTEIF3  |
                DMA_LIFCR_CDMEIF3 |
                DMA_LIFCR_CFEIF3;
    bq->bufferFilled(bufferSize);
    IRQdmaRefill();
    waiting->IRQwakeup();
    if(waiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
        Scheduler::IRQfindNextThread();
}

/**
 * This function allows to atomically check if a variable equals a specific
 * value and if not, put the thread in wait state until said condition is
 * satisfied. To wake the thread, another thread or an interrupt routine
 * should first set the variable to the desired value, and then call
 * wakeup() (or IRQwakeup()) on the sleeping thread.
 * \param T type of the variable to test
 * \param variable the variable to test
 * \param value the value that will cause this function to return.
 */
template<typename T>
static void atomicTestAndWaitUntil(volatile T& variable, T value)
{
    FastInterruptDisableLock dLock;
    while(variable!=value)
    {
        Thread::IRQgetCurrentThread()->IRQwait();
        {
            FastInterruptEnableLock eLock(dLock);
            Thread::yield();
        }
    }
}

/**
 * Helper function that waits until a buffer is available for reading
 * \return a readable buffer from bq
 */
static const unsigned short *getReadableBuffer()
{
    FastInterruptDisableLock dLock;
    const unsigned short *result;
        unsigned int size;
    while(bq->tryGetReadableBuffer(result, size)==false)
    {
        waiting->IRQwait();
        {
            FastInterruptEnableLock eLock(dLock);
            Thread::yield();
        }
    }
    return result;
}

static void bufferEmptied()
{
    FastInterruptDisableLock dLock;
    bq->bufferEmptied();
}


Microphone& Microphone::instance()
{
    static Microphone singleton;
    return singleton;
}

Microphone::Microphone() {}


void Microphone::init(function<void (unsigned char*, int)> cback){
    callback = cback;
    PCMsize = SAMPLES_AT_TIME * DECIMATION_FACTOR;
    compressed_buf_size_bytes = (PCMsize/DECIMATION_FACTOR) / 2;
}

void Microphone::start(){
    recording = true;
    readyBuffer = (short*) malloc(sizeof(short) * PCMsize);
    processingBuffer = (short*) malloc(sizeof(short) * PCMsize);
    // here I put 1 sample every DECIMATION_FACTOR - 1 samples
    decimatedReadyBuffer = (short*) malloc(sizeof(short) * PCMsize / DECIMATION_FACTOR);
    decimatedProcessingBuffer = (short*) malloc(sizeof(short) * PCMsize / DECIMATION_FACTOR);

    compressedBuf = (unsigned char*) malloc(compressed_buf_size_bytes*sizeof(char));
    {
        FastInterruptDisableLock dLock;
        //Enable DMA1 and SPI2/I2S2 and GPIOB and GPIOC
        RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
        RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;   
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN;
        
        //Configure GPIOs
        clk::mode(Mode::ALTERNATE);
        clk::alternateFunction(5);
        clk::speed(Speed::_50MHz);
        dout::mode(Mode::ALTERNATE);
        dout::alternateFunction(5);
        dout::speed(Speed::_50MHz);
        
        
        // I2S PLL Clock Frequency: 135.5 Mhz
        RCC->PLLI2SCFGR=(2<<28) | (271<<6);
        
        RCC->CR |= RCC_CR_PLLI2SON;
    }
    //Wait for PLL to lock
    while((RCC->CR & RCC_CR_PLLI2SRDY)==0) ;
    
    // RX buffer not empty interrupt enable
    SPI2->CR2 = SPI_CR2_RXDMAEN;  
    
    SPI2->I2SPR=  SPI_I2SPR_MCKOE | 12;

    //Configure SPI
    SPI2->I2SCFGR = SPI_I2SCFGR_I2SMOD | SPI_I2SCFGR_I2SCFG_0 | SPI_I2SCFGR_I2SCFG_1 | SPI_I2SCFGR_I2SE;

    NVIC_SetPriority(DMA1_Stream3_IRQn,2);//High priority for DMA

    delayMs(10); //waits for the microphone to enable
    pthread_create(&mainLoopThread,NULL,mainLoopLauncher,reinterpret_cast<void*>(this));
}

void* Microphone::mainLoopLauncher(void* arg){
        reinterpret_cast<Microphone*>(arg)->mainLoop();
}

void Microphone::mainLoop(){

    waiting = Thread::getCurrentThread();
    pthread_t cback;
    bq=new BufferQueue<unsigned short,bufferSize,bufNum>();
    NVIC_EnableIRQ(DMA1_Stream3_IRQn);  
    // create the thread that will execute the callbacks 
    pthread_create(&cback,NULL,callbackLauncher,reinterpret_cast<void*>(this));
    isBufferReady = false;
    // variable used for swap of processing and ready buffer
    short *tmp, *decimatedTmp;

    while(recording){
        PCMindex = 0;      
        // process any new chunk of PDM samples
        for (;;){
            if(enobuf){
                enobuf = false;
                dmaRefill();
            }
            if(processPDM(getReadableBuffer(),bufferSize) == true){
                // transcode until the specified number of PCM samples
                break;
            }
            bufferEmptied();  

        }
        // swaps the ready and the processing buffer: allows double buffering
        // on the callback side
        tmp = readyBuffer;
        decimatedTmp = decimatedReadyBuffer;

        readyBuffer = processingBuffer;
        decimatedReadyBuffer = decimatedProcessingBuffer;
        // perform encoding using ADPCM codec
        encode(&state, decimatedReadyBuffer, PCMsize/DECIMATION_FACTOR, compressedBuf);

        // start critical section
        pthread_mutex_lock(&bufMutex);
        isBufferReady = true;
        pthread_cond_broadcast(&cbackExecCond);
        pthread_mutex_unlock(&bufMutex);
        // end critical section
        processingBuffer = tmp;
        decimatedProcessingBuffer = tmp;

    }
    pthread_cond_broadcast(&cbackExecCond);
    pthread_join(cback, NULL);
}

void* Microphone::callbackLauncher(void* arg){
    reinterpret_cast<Microphone*>(arg)->execCallback();
}

void Microphone::execCallback() {
    while(recording){
        pthread_mutex_lock(&bufMutex);
        while(recording && !isBufferReady)
            pthread_cond_wait(&cbackExecCond, &bufMutex);
        callback(compressedBuf,compressed_buf_size_bytes);
        isBufferReady = false;
        pthread_mutex_unlock(&bufMutex);
    }
}

bool Microphone::processPDM(const unsigned short *pdmbuffer, int size) {
    int decimatedIndex;
    int remaining = PCMsize - PCMindex;
    int length = std::min(remaining, size); 
    short int s, sHigh, valueBeforeJump, jumpValue;
    bool jumpMade = false;

    for (int i=0; i < length; i++){    

        // convert couples 16 pdm one-bit samples in one signed 16-bit PCM sample
        // -32768 is because microphone outputs unsigned samples
        // so I translate each sample of half of the range
        s = PDMFilter(pdmbuffer, i) - 32768;
        // check if this sample belongs to a noise peak or not
        if(PCMindex - 1 >= 0) {
            if(!jumpMade && s - processingBuffer[PCMindex - 1] > 30000){
                jumpMade = true;
                sHigh = s;
                valueBeforeJump = processingBuffer[PCMindex - 1];
                jumpValue = s - valueBeforeJump;
            }
            else if (jumpMade && sHigh - s > 30000)
                jumpMade = false;
        }
        // if the sample is not in a noise-generated peak
        if (!jumpMade){
            // perform decimation
            processingBuffer[PCMindex] = s;
            decimatedIndex = PCMindex / DECIMATION_FACTOR;
            if(PCMindex - (decimatedIndex * DECIMATION_FACTOR) == 0){
                decimatedProcessingBuffer[decimatedIndex] = s;
            }
        }
        PCMindex++;
    }
    if (PCMindex < PCMsize) //if produced PCM sample are not enough 
        return false; 
    return true;    
}


/*
 * This function takes care of the transcoding from 16 PDM bit to 1 PCM sample
 * via CIC filtering. Decimator rate: 16:1, CIC stages: 4.
 */
unsigned short Microphone::PDMFilter(const unsigned short* PDMBuffer, unsigned int index) {
    short combInput, combRes;
    
    // perform integration on the first word of the PDM chunk to be filtered
    for (short i=0; i < 16; i++){
        //integrate each single bit
        intReg[0] += pdmLUT[(PDMBuffer[index] >> (15-i)) & 1];
        for (short j=1; j < filterOrder; j++){
            intReg[j] += intReg[j-1];
        }
    }
    
    // the last cell of intReg contains the output of the integrator stage
    combInput = intReg[filterOrder-1];
    
    //apply the comb filter (with delay 1):
    for (short i=0; i < filterOrder; i++){
        combRes = combInput - combReg[i];
        combReg[i] = combInput;
        combInput = combRes;
    }
    
    return combRes;
    
}

Microphone::Microphone(const Microphone& orig) {
}

void Microphone::stop() {
    // stop the software
    recording = false;
    // waits for the last PCM processing to end
    pthread_join(mainLoopThread, NULL);
    // reset the configuration registers to stop the hardware
    NVIC_DisableIRQ(DMA1_Stream3_IRQn);
    delete bq;
    SPI2->I2SCFGR=0;
    {
    FastInterruptDisableLock dLock;
        RCC->CR &= ~RCC_CR_PLLI2SON;
    }

    // sends ending signal to desktop script
    // ATTENTION: it's application specific, you might not need it
    unsigned char *uselessBuffer;
    callback(uselessBuffer,0);
    
    free(readyBuffer);
    free(processingBuffer);
    free(decimatedProcessingBuffer);
    free(decimatedReadyBuffer);
}

int Microphone::getBatchSize(){
    return compressed_buf_size_bytes;
}

Microphone::~Microphone() {
}


