/*
 * Basic check of frequency range and sounds
 *
 *
 *
 *
*******************************************************************************/
/* DriverLib Includes */
#include "driverlib.h"

/* Standard Includes */
#include <stdint.h>
#include <stdbool.h>

// Functions declared after main

#define BUTTON_PORT P5
#define BUTTON_BIT  BIT1



volatile float initialHalfPeriod = 1; //(500000*(1/(JoyToTheWorld[increment])))
volatile float frequency;
volatile int sound;

void init_timer(void);
void init_button(void);

static uint16_t resultsBuffer[2]; //ADC results
/*
 * These timer registers reflect most of the state of the application:
 * TA0CCR0 = half period for PWM and timebase for TA0.0 interrupts = 99 (fixed)
 * TA0CCR4 = duty cycle for pwm (varied by the button interrupt handler)
 * TIMER_A0->CCTL[4] contains the outmod bits which will be toggled between
 *                   mode 0 and reset-set by the TA0.0 handler
 *
 * The TA0.0 interrupt handler does several time related housekeeping functions by running
 * down counters to take an action:
 *
 * VARIABLE				ACTION when variable=0
 * counter				toggle the TA0.4 output mode between reset/set and fixed output
 * debounce_counter		button handler actually takes action
 *
 * PARAMETERS:
 * PERIOD = TACCR0 value (unchanged in the program)
 * DUTY_CYCLE_STEP = how much TA0CCR4 is incrememented each button push
 * INITIAL_DUTY_CYCLE= initial value of the duty cycle (TA0CCR4)
 * AUDIO_COUNT = number of TA0.0 intervals that expire before TA0.0 handler toggles PWM signal
 * DEBOUNCE_COUNT = minimum number of TA0.0 intervals before button presses acted on
 *
 */
#define y_min 0
#define x_min 25

#define y_max 100
#define x_max 250



#define PERIOD 100
#define DUTY_CYCLE_STEP 10
#define INITIAL_DUTY_CYCLE 10
#define DEBOUNCE_COUNT 2000
// This is 20ms for debouncing...
volatile int duty_cycle;
volatile int AUDIO_COUNT;
int main(void)
{
	sound=1;
    /* Stop Watchdog  */
    MAP_WDT_A_holdTimer();
    AUDIO_COUNT=50; //	just initial value

    //copied from ADC joystick example
    	 /* Configures Pin 6.0 and 4.4 as ADC input */
    	    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P6, GPIO_PIN0, GPIO_TERTIARY_MODULE_FUNCTION);
    	    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P4, GPIO_PIN4, GPIO_TERTIARY_MODULE_FUNCTION);

    	    /* Initializing ADC (ADCOSC/64/8) */
    	    MAP_ADC14_enableModule();
    	    MAP_ADC14_initModule(ADC_CLOCKSOURCE_ADCOSC, ADC_PREDIVIDER_64, ADC_DIVIDER_8, 0);

    	    /* Configuring ADC Memory (ADC_MEM0 - ADC_MEM1 (A15, A9)  with repeat)
    	         * with VCC reference */
    	    MAP_ADC14_configureMultiSequenceMode(ADC_MEM0, ADC_MEM1, true);
    	    MAP_ADC14_configureConversionMemory(ADC_MEM0,
    	            ADC_VREFPOS_AVCC_VREFNEG_VSS,
    	            ADC_INPUT_A15, ADC_NONDIFFERENTIAL_INPUTS);

    	    MAP_ADC14_configureConversionMemory(ADC_MEM1,
    	            ADC_VREFPOS_AVCC_VREFNEG_VSS,
    	            ADC_INPUT_A9, ADC_NONDIFFERENTIAL_INPUTS);

    	    /* Enabling the interrupt when a conversion on channel 1 (end of sequence)
    	     *  is complete and enabling conversions */
    	    MAP_ADC14_enableInterrupt(ADC_INT1);

    	    /* Enabling Interrupts */
    	    MAP_Interrupt_enableInterrupt(INT_ADC14);
    	    MAP_Interrupt_enableMaster();

    	    /* Setting up the sample timer to automatically step through the sequence
    	     * convert.
    	     */
    	    MAP_ADC14_enableSampleTimer(ADC_AUTOMATIC_ITERATION);

    	    /* Triggering the start of the sample */
    	    MAP_ADC14_enableConversion();
    	    MAP_ADC14_toggleConversionTrigger();
    	//end copy




    /* Set DCO speed */
    /* since this is done once, no need to turn on FPU for efficiency */
    MAP_CS_setDCOFrequency(10000000);  /* 10MHz */

    init_button();
    init_timer();

	MAP_Interrupt_disableSleepOnIsrExit();   // Specify that after an interrupt, the CPU wakes up

	MAP_Interrupt_enableMaster();// unmask IRQ interrupts to allow the CPU to respond.
	NVIC->ISER[1] = 1 << ((PORT5_IRQn) & 31); // enable P5 to send interrupt signals
    while(1)
    {
        /* Go to LPM0 mode (Low power mode with CPU powered off */
        MAP_PCM_gotoLPM0();		  //
        __no_operation(); //  For debugger
    }
}

/*
 * PWM and sound production system using TA0.0 and TA0.4
 */


// TA0CCR0 Interrupt Handler

// Variables used for time related activities by the handler

volatile unsigned int counter; // down counter for toggling pwm output mode
volatile unsigned int debounce_counter; // down counter for resetting button_flag

void TA0_0_Handler(){

		MAP_Timer_A_clearCaptureCompareInterrupt(TIMER_A0_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_0);
		if (--counter == 0){
			TIMER_A0->CCTL[4]^=TIMER_A_CCTLN_OUTMOD_7; // toggle between mode 0 and mode 7 reset/set
			counter=AUDIO_COUNT;
		}
		if (debounce_counter) {  // are we still debouncing last button press?
			--debounce_counter==0;
		}

}


/* Timer_A UpMode Configuration Parameter */
const Timer_A_UpModeConfig upConfig =
{
        TIMER_A_CLOCKSOURCE_SMCLK,              // SMCLK Clock Source
        TIMER_A_CLOCKSOURCE_DIVIDER_1,          // 10MHz
		PERIOD-1,                               //
        TIMER_A_TAIE_INTERRUPT_DISABLE,         // Disable Timer interrupt
        TIMER_A_CCIE_CCR0_INTERRUPT_DISABLE ,   // Disable CCR0 interrupt
        TIMER_A_DO_CLEAR                        // Clear value
};
const Timer_A_CompareModeConfig ccr0_Config ={
		TIMER_A_CAPTURECOMPARE_REGISTER_0,
		TIMER_A_CAPTURECOMPARE_INTERRUPT_ENABLE,
		TIMER_A_OUTPUTMODE_OUTBITVALUE,
		PERIOD-1
};

const Timer_A_CompareModeConfig ccr4_Config ={
		TIMER_A_CAPTURECOMPARE_REGISTER_4,
		TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE,
		TIMER_A_OUTPUTMODE_RESET_SET,
		INITIAL_DUTY_CYCLE
};

void init_timer(){              // initialization and start of timer
    /* Configuring Timer_A1 for Up Mode */
    MAP_Timer_A_configureUpMode(TIMER_A0_BASE, &upConfig);

    // configure channels
    MAP_Timer_A_initCompare(TIMER_A0_BASE, &ccr0_Config);
    MAP_Timer_A_initCompare(TIMER_A0_BASE, &ccr4_Config);

    counter=AUDIO_COUNT;

    MAP_Timer_A_registerInterrupt(TIMER_A0_BASE,TIMER_A_CCR0_INTERRUPT,TA0_0_Handler);

    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2,GPIO_PIN7,GPIO_PRIMARY_MODULE_FUNCTION);
	MAP_Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE); // start TA0 in up mode
}



/*
 * Button input System
 *
 * The Button toggles the state of the sound (on or off)
 * Action will be interrupt driven on a down going signal on the pin
 * no debouncing in this code, but MSP432P401R has glitch suppression which may help.
 */

void PORT5_IRQHandler(){
	if (BUTTON_PORT->IFG & BUTTON_BIT){ // check that it is the button interrupt flag
			BUTTON_PORT->IFG &= ~BUTTON_BIT; // clear the flag to allow for another interrupt later.
			// This handler changes the state of the timer CCTL0 control register!
			// Toggle OUTMOD between
			//    mode 0: output = constant value determined by CCTL0 bit 2, and
			//    mode 4: toggle, which when repeated produces a square wave.
			switch (sound) {
					case 0:{
						sound=1;
						break;
					}
					case 1:{
						sound=0;
						break;
					}
				}
		}
}

void init_button(){
// All GPIO's are already inputs if we are coming in after a reset
	MAP_GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P5,GPIO_PIN1);
	MAP_GPIO_interruptEdgeSelect(GPIO_PORT_P5,GPIO_PIN1,GPIO_HIGH_TO_LOW_TRANSITION);
	MAP_GPIO_clearInterruptFlag(GPIO_PORT_P5,GPIO_PIN1);
	debounce_counter=0; // timer does not need to reset the flag
	MAP_GPIO_registerInterrupt(GPIO_PORT_P5,PORT5_IRQHandler); //registers handler with NVIC
	MAP_GPIO_enableInterrupt(GPIO_PORT_P5,GPIO_PIN1);
}


void ADC14_IRQHandler(void)
{
    uint64_t status;
    status = MAP_ADC14_getEnabledInterruptStatus();
    MAP_ADC14_clearInterruptFlag(status);

    /* ADC_MEM1 conversion completed */
    if(status & ADC_INT1)
    {
        /* Store ADC14 conversion results */
       resultsBuffer[0] = ADC14_getResult(ADC_MEM0);
       resultsBuffer[1] = y_min+ADC14_getResult(ADC_MEM1)*y_max/16383;	// range from 0 to 99

       frequency=resultsBuffer[0];
       initialHalfPeriod=(500000*(1/(frequency)));

       TIMER_A0->CCR[0] = initialHalfPeriod;


       if(sound==1){
       MAP_Timer_A_setCompareValue(TIMER_A0_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_4,resultsBuffer[1]);	//	writes joystick input to volume control
       }
       else{
    	   MAP_Timer_A_setCompareValue(TIMER_A0_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_4,0);
       }
       AUDIO_COUNT=x_min+resultsBuffer[0]*x_max/16383;					// 200 - 2000 hz or ~ (G#3/Ab3 to B6)

        /* Determine if JoyStick button is pressed */
       /*
        int buttonPressed = 0;
        if (!(P4IN & GPIO_PIN1))
            buttonPressed = 1;
            */
    }


}


