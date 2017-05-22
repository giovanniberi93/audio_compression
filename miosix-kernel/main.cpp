#include <cstdio>
#include "miosix.h"
#include "Microphone.h"
#include "button.h"
#include <math.h>
#include <functional>
#include <termios.h>
#include <string.h>

using namespace std;
using namespace miosix;


void sendToSerial(unsigned char* compressedData, unsigned int size){
    // send actual size of the batch
    write(STDOUT_FILENO,&size,sizeof(int));
    // send the batch of data
    write(STDOUT_FILENO,compressedData,size);
}


// configure stdout in raw mode
void setRawStdout(){
    struct termios t;
    tcgetattr(STDOUT_FILENO,&t);
    t.c_lflag &= ~(ISIG | ICANON); 
    tcsetattr(STDOUT_FILENO,TCSANOW, &t); 
}


void sendInitSignal(int expectedBatchSize){
    // send signal to desktop script
    char init[] = "ready\n";
    write(STDOUT_FILENO,init,strlen(init));
    // send expected size of the data batches
    write(STDOUT_FILENO,&expectedBatchSize,sizeof(int));
}


int main()
{
    Microphone& mic = Microphone::instance(); 
    mic.init(bind(sendToSerial,placeholders::_1,placeholders::_2));
    buttonInit();
    setRawStdout();

    waitForButton();
    sendInitSignal(mic.getBatchSize());
    mic.start();
    ledOn();

    // ending procedure
    waitForButton();
    mic.stop();
    ledOff();

    while (1){
    };
    
}
