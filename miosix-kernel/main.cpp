
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

void visualize(unsigned short* PCM, unsigned short  size){
    // if(PCM[1] > 50000)
    //     orangeLed::high();
    // else
    //     orangeLed::low();
    fwrite(PCM, sizeof(unsigned short int), size, stdout);
}

void setRawStdout(){
    // configure stdout in raw mode
    struct termios t;
    tcgetattr(STDOUT_FILENO,&t);
    t.c_lflag &= ~(ISIG | ICANON);    
}

int main()
{

    orangeLed::mode(Mode::OUTPUT);
    setRawStdout();
    /* Best results obtained with a size in the form of (N + 0.5) * 256 with N integer */
    static const unsigned short size = 10;

    Microphone& mic = Microphone::instance(); 
    mic.init(bind(&visualize,placeholders::_1,placeholders::_2), size);
    buttonInit();

    // starting procedure
    waitForButton();
    mic.start();
    ledOn();
    // ending procedure
    waitForButton();
    mic.stop();
    ledOff();

    while (1){
    };
    
}
