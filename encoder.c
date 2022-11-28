
/* Header files */
#include "driverlib.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/* Header files */
#define TIME_PERIOD 2000

/* Function declarations */
void declareTimer(void);
void declarePins(void);
void declareInterrupts (void);
void PORT3_IRQHandler(void);
void TA2_0_IRQHandler(void);


const int notchTOrotation= 20;

volatile int timeTaken = 0;

volatile int leftNotches= 0;
volatile int rightNotches= 0;
volatile int leftCounter= 0;
volatile int rightCounter= 0;


void declareTimer (void)
{
    WDT_A_holdTimer();

    // Set up Timer_A2, used for interrupt count check, (1MHz clock).
    const Timer_A_UpModeConfig upConfig = {
        TIMER_A_CLOCKSOURCE_SMCLK,          // SMCLK Clock Source
        TIMER_A_CLOCKSOURCE_DIVIDER_3,      // SMCLK/3 = 1MHz
        TIME_PERIOD,                 // 2000 tick period
        TIMER_A_TAIE_INTERRUPT_DISABLE,     // Disable Timer interrupt
        TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE, // Enable CCR0 interrupt
        TIMER_A_DO_CLEAR                    // Clear value
    };
    Timer_A_configureUpMode(TIMER_A2_BASE, &upConfig);
    Timer_A_clearTimer(TIMER_A2_BASE);
}
void declarePins (void)
{
    // Left Encoder & Right Encoder
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P3, GPIO_PIN2 | GPIO_PIN3);

// Select edge that triggers the interrupt
    GPIO_interruptEdgeSelect(
        GPIO_PORT_P3, GPIO_PIN2 | GPIO_PIN3, GPIO_LOW_TO_HIGH_TRANSITION);
}
void declareInterrupts (void)
{
    // Clear pin's interrupt flag for P3.2 & P3.3
    GPIO_clearInterruptFlag(GPIO_PORT_P3, GPIO_PIN2 | GPIO_PIN3);

    // Enable interrupt bit of P3.2 & P3.3
    GPIO_enableInterrupt(GPIO_PORT_P3, GPIO_PIN2 | GPIO_PIN3);

// Set interrupt enable (IE) bit of corresponding interrupt source
    Interrupt_enableInterrupt(INT_PORT3);
    Interrupt_enableInterrupt(INT_TA2_0);
}
void PORT3_IRQHandler(void)
{
    uint32_t status;

    status = GPIO_getEnabledInterruptStatus(GPIO_PORT_P3);

    if (status & GPIO_PIN2)
    {
       notchesDectected++;
	leftNotches = notchesDectected;
	if(leftNotches % notchTOrotation == 0) 
	{
		leftCounter++; 
	}
}
    if (status & GPIO_PIN3)
    {
       notchesDectected++;
	rightNotches = notchesDectected;
	if(rightNotches % notchTOrotation == 0) 
	{
		rightCounter++; 
	}
    }

    GPIO_clearInterruptFlag(GPIO_PORT_P3, GPIO_PIN2 | GPIO_PIN3);
    
}
void TA2_0_IRQHandler(void)
{
    if (timeTaken % 5000 == 0) //check every 5seconds
    {
        leftTimer++;
        rightTimer++;
    }
    Timer_A_clearCaptureCompareInterrupt(TIMER_A2_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0);
}
void getSpeed(void)
{
     speed = ((leftNotches + rightNotches)/2)/(leftTimer + rightTimer)
     return speed;	
} 
