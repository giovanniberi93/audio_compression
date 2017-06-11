/**************************************************************************
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
