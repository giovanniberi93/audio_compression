// write e raw
// /////sdpcm//// adpcm
// bitvuket /aturri/btphone


#include <cstdio>
#include "miosix.h"
#include "Microphone.h"
#include "button.h"
#include <math.h>
#include <functional>
#include <termios.h>

using namespace std;
using namespace miosix;

typedef Gpio<GPIOD_BASE,13> orangeLed;


unsigned short b[] = {10,10,10,10,10,10,10,10};
// unsigned short b[] = {10};

void sendToSerial(unsigned short* PCM, unsigned short size){
        write(STDOUT_FILENO,b,sizeof(unsigned short)*size);
        // fflush(stdout); 

}

// configure stdout in raw mode
void setRawStdout(){
    struct termios t;
    tcgetattr(STDOUT_FILENO,&t);
    t.c_lflag &= ~(ISIG | ICANON); 
    tcsetattr(STDOUT_FILENO,TCSANOW, &t); 
}


// RICORDATI CHE HAI ATTIVATO IL FLOW CONTROL DA BOARD_SETTINGS.H
// simple handshake with desktop script
void serialHandshake(int size){
    iprintf("new\n");
    iprintf("%d\n",size);
}

int main()
{
    orangeLed::mode(Mode::OUTPUT);
    setRawStdout();
    // configSerial();
    // Best results obtained with a size in the form of (N + 0.5) * 256 with N integer
    static const unsigned short size = 8;

    Microphone& mic = Microphone::instance(); 
    mic.init(bind(sendToSerial,placeholders::_1,placeholders::_2), size);
    buttonInit();

    // starting procedure
    waitForButton();
    serialHandshake(size);
    mic.start();
    ledOn();
    // ending procedure
    waitForButton();
    mic.stop();
    ledOff();

    while (1){
    };
    
}
