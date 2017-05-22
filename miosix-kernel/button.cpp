
#include "button.h"
#include <miosix.h>
#include <miosix/kernel/scheduler/scheduler.h>

using namespace miosix;

typedef Gpio<GPIOA_BASE,0> button;

static Thread *waiting=0;
int hasJustPushed = 0;

void __attribute__((naked)) EXTI0_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z16EXTI0HandlerImplv");
    restoreContext();
}


void __attribute__((used)) EXTI0HandlerImpl()
{
    EXTI->PR=EXTI_PR_PR0;
    if(waiting==0 || hasJustPushed) return;
    // task become schedulable again
    waiting->IRQwakeup();
    if(waiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
		Scheduler::IRQfindNextThread();
    waiting=0;
}

void buttonInit()
{
    button::mode(Mode::INPUT_PULL_DOWN);
    EXTI->IMR |= EXTI_IMR_MR0;
    EXTI->RTSR |= EXTI_RTSR_TR0; 
    NVIC_EnableIRQ(EXTI0_IRQn);
    NVIC_SetPriority(EXTI0_IRQn,15); //Low priority
}

// task
void waitForButton()
{
    // disable interrupt
    FastInterruptDisableLock dLock;
    waiting=Thread::IRQgetCurrentThread();
    while(waiting)
    {

        // call a preemption, return only when someone wake me up (sw interrupt
        Thread::IRQwait();
        // enable interrupt only in the scope
        FastInterruptEnableLock eLock(dLock);
        // dont return until someone wake me up
        Thread::yield();
    }
    signalHasJustPushed();
}

void signalHasJustPushed(){
    hasJustPushed = 1;
    delayMs(200); 
    hasJustPushed = 0;
}
