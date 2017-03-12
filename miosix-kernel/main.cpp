
#include <cstdio>
#include "miosix.h"
#include "Microphone.h"
#include "button.h"
#include <math.h>
#include <functional>

using namespace std;
using namespace miosix;


void visualize(unsigned short* PCM, unsigned short  size){
    iprintf("new\n");
    for(int i = 0; i < size; i++){
        // outputs unsigned value of the samples
        iprintf("%d\n",(short) PCM[i] );
    }
}

int main()
{
    Microphone& mic = Microphone::instance(); 

    /* Best results obtained with a size in the form of (N + 0.5) * 256 with N integer */
    static const unsigned short size = 384;
    // mic.init(&visualize,size);
    mic.init(bind(&visualize,placeholders::_1,placeholders::_2), size);
    buttonInit();

    waitForButton();
    mic.start();
    ledOn();

    waitForButton();
    mic.stop();
    ledOff();

    while (1){
    };
    
}
