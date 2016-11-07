


#include "msp.h"

/* Variables used by the WDT handler must maintain their values between interrupts.
 * The easiest way to achieve this is to declare them globally
 */


//----------------------------------
// Logical Names and Parameters
// DEFINE MUSIC NOTES FOR READABLILITY

// half period for a 1KHz tone.
#define initialHalfPeriod 500




// declare functions defined below

void mapports();         // connect TACCR0 to P2.7 using PMAP
void init_timer(void);   // routine to setup the timer
void init_button(void);  // routine to setup the button

volatile unsigned char last_button_1; 			//	this is 0 or BIT1 (ie bit 1 of P5)
volatile unsigned char last_button_2; 			//	this is 0 or BIT5 (ie bit 5 of P3)

volatile unsigned char state;					//	state is 0 or 1
volatile unsigned char sound;					//	state is 0 or 1

volatile unsigned int half_period=183;			//	this can be changed in the debugger


unsigned int i, j;
unsigned int delay=60;
unsigned int s_1_size=147;
unsigned int s_2_size=231;
/*


volatile short int song_1[24] = {	C5,C5,C5,C5, B4,B4,B4,A4,
						G4,G4,G4,G4,G4,G4,F4,F4,
						E4,E4,E4,E4,D4,D4,D4,D4};
short int C5 =956;
short int B4 =1012;
short int A4 =1136;
short int G4 =1276;
short int F4 =1432;
short int E4 =1517;
short int D4 =1703;
short int C4 =1911;
*/

short int song_1[147]={	956,956,956,956,		1012,1012,1012,1136,
						1276,1276,1276,1276,	1276,1276,1432,1432,
						1517,1517,1517,1517,	1703,1703,1703,1703,
						1911,1911,1911,1911,	1911,1911,1276,1276,
						1136,1136,1136,1136,	1136,1136,1136,1136,
						1012,1012,1012,1012,	1012,1012,1012,1012,
						956,956,956,956,		956,956,956,956,
						956,956,1012,1012,		1136,1136,1276,1276,
						1276,1276,1276,1432,	1517,1517,956,956,
						956,956,1012,1012,		1136,1136,1276,1276,
						1276,1276,1276,1432,	1517,1517,1517,1517,
						1517,1517,1517,1517,	1517,1517,1517,1432,
						1276,1276,1276,1276,	1276,1276,1432,1517,
						1703,1703,1703,1703,	1703,1703,1703,1517,
						1432,1432,1432,1432,	1432,1432,1517,1703,
						1911,1911,956,956,		956,956,1136,1136,
						1276,1276,1276,1432,	1517,1517,1432,1432,
						1517,1517,1517,1517,	1703,1703,1703,1703,
						1703,1703,1703
						};

short int song_2[232]={
						196,196,196,196,		1276,1276,1276,1276,
						1432,1432,1432,1432,	1517,1517,1517,1517,
						1804,1804,1804,1804,	1703,1703,1703,1703,
						1136,1136,1136,1136,	1136,1136,1136,1136,
						2551,2551,2551,2551,	1012,1012,1012,1012,
						1136,1136,1136,1136,	1276,1276,1276,1276,
						1351,1351,1351,1351,	1276,1276,1276,1276,
						956,956,956,956,		956,956,956,956,
						851,851,851,851,		956,956,956,956,
						1012,1012,1012,1012,	1136,1136,1136,1136,
						1276,1276,1276,1276,	1432,1432,1432,1432,
						1517,1517,1517,1517,	1703,1703,1703,1703,
						1136,1136,1136,1136,	1136,1136,1136,1136,
						2025,2025,2025,2025,	2025,2025,2025,2025,
						1276,1276,1276,1276,

						196,196,196,196,		1276,1276,1276,1276,
						1432,1432,1432,1432,	1517,1517,1517,1517,
						1804,1804,1804,1804,	1703,1703,1703,1703,
						1136,1136,1136,1136,	1136,1136,1136,1136,
						2551,2551,2551,2551,	1012,1012,1012,1012,
						1136,1136,1136,1136,	1276,1276,1276,1276,
						1351,1351,1351,1351,	1276,1276,1276,1276,
						956,956,956,956,		956,956,956,956,
						851,851,851,851,		956,956,956,956,
						1012,1012,1012,1012,	1136,1136,1136,1136,
						1276,1276,1276,1276,	1432,1432,1432,1432,
						1517,1517,1517,1517,	1703,1703,1703,1703,
						1136,1136,1136,1136,	1136,1136,1136,1136,
						2025,2025,2025,2025,	2025,2025,2025,2025,
						1276,1276,1276,20

						};

// Global variable for data, how long to wait before toggling blue light


/*
 * a procedure to change states (called by the WDT handler)
 */

void change_state(){
	switch (state) {
		case 0:{ 								//	now toggling green, change to steady RED
			state=1;							//	move back to REC mode

			P2->OUT&= ~BIT4; 					//	turn off green LED
			P2->OUT |= BIT6; 					//	turn on RED LED

			P2->OUT &= ~BIT2;        			//  P2.2 BLUE LED OFF as initial state

			i=0;								//	reset song timers if song states changes
			j=0;


			break;
		}
		case 1:{								// now steady RED, change to STEADY green
			state=0;							// move back to PLAY mode
			P2->OUT &= ~BIT6;					// turn off RED
			P2->OUT |= BIT4;					// turn on green

			P2->OUT &= ~BIT2;        			//  P2.2 BLUE LED OFF as initial state

			i=0;
			j=0;

			break;
		}
	}
}

void change_sound(){
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





/*
 * Watchdog Timer interrupt service routine
 * (Note: the function name for this handler is declared as an external routine in one of the
 * automatically loaded startup c files from the Code Composer system).
 */
void WDT_A_IRQHandler(void)
{

	/* Check if the SONG button was pressed since last interrupt */
	unsigned char b= P3->IN & BIT5 ; 			//	read the button
	if ((b==0) && last_button_2){				//	did we go from unpressed(1) to pressed(0)?
		change_state();							//	called when STATES ARE CHANGING
	}
	last_button_2=b;							//	remember the current state of the button


	unsigned char a= P5->IN & BIT1 ; 			//	read the button
	if ((a==0) && last_button_1){				//	did we go from unpressed(1) to pressed(0)?
		change_sound();							//	called when STATES ARE CHANGING
	}
	last_button_1=a;							//	remember the current state of the button


	if(state==1){							//	in SONG 1 mode
		P2->OUT&= ~BIT4; 					//	turn off green LED
		P2->OUT |= BIT6; 					//	turn on RED LED
											//	reestablish color if overpress cause state change

		if(i==delay){
					i=0;
					if(j>s_2_size){
						TIMER_A0->CCTL[0] &= ~TIMER_A_CCTLN_OUTMOD_4;
					}
					else{
						TIMER_A0->CCR[0] = song_2[j];
						if(sound!=0){
						j=j+1;
						}
					}
				}
				else{
					i=i+1;
				}
	}


	else if(state==0){						//	in SONG 1 mode

		P2->OUT &= ~BIT6;					//	turn off RED
		P2->OUT |= BIT4;					//	turn on green

		if(i==delay){
			i=0;
			if(j>s_1_size){
				TIMER_A0->CCTL[0] &= ~TIMER_A_CCTLN_OUTMOD_4;
			}
			else{
				TIMER_A0->CCR[0] = song_1[j];
				if(sound!=0){
				j=j+1;
				}
			}
		}
		else{
			i=i+1;
		}


	}
	else{
	}



	if(sound==1){								//	Set, on
		TIMER_A0->CCTL[0] |= TIMER_A_CCTLN_OUTMOD_4;
		P2->OUT |= BIT2;				//	ON blue light
		}
	else if(sound==0){							// CLEAR, off
		TIMER_A0->CCTL[0] &= ~TIMER_A_CCTLN_OUTMOD_4;
		P2->OUT &= ~BIT2;        		//  P2.2 BLUE LED OFF  state
		}
	else{
		}




}

/*
 * The main program just initializes everything and (more or less) waits for events
 */

void main(void)
{


    /* Configure watch dog -- see the documentation on the WDT control register
     * - SMCLK as clock source
     * - Interval timer mode
     * - Clear WDT counter (initial value = 0)
     * - Watchdog interval = 8K = 2^13 ticks
     *      so that the interval is ~ 8K/(3Mhz) ~ 2.7ms
     */
    WDT_A->CTL = WDT_A_CTL_PW |				// 'password' to enable access
            WDT_A_CTL_SSEL__SMCLK |         // clock source = SMCLK
            WDT_A_CTL_TMSEL |               // this bit turns on interval mode
            WDT_A_CTL_CNTCL |               // clear the internal WDT counter
            WDT_A_CTL_IS_5;                 // specifies the divisor.  value 5 => 8K



	mapports();
	init_timer();  // initialize timer

    // Setup the output pins
    P2->DIR |= BIT4;   // Direction is output
    P2->OUT &= ~BIT4;  // Green LED off to start with

    P2->DIR |= BIT6;   // Direction is output
    P2->OUT &= ~BIT6;  // RED LED off to start with

	P2->DIR |= BIT2;   // Direction is output
    P2->OUT &= ~BIT2;  // BLUE LED off to start with


    //The button should be set to input after reset, but we do it here as an example
    P3->DIR &=~BIT5;   					//	clear the direction. set bit 5 to 0 and INPUT
    last_button_2 = P3->IN & BIT5; 		//	initialize the previous state of the button (needed by handler)

	P5->DIR &=~BIT1;					//	clear the direction. set bit 1 to 0 and INPUT
	last_button_1 = P5->IN & BIT1;		//	initialize the previous state of the button (needed by handler)


	state=0;							//	starts off in song 1
	sound=1;							//	Sound on
	i=0;
	j=0;
    /*
     * setup so that the system sleeps until interrupted
     */

    SCB->SCR &= ~SCB_SCR_SLEEPONEXIT_Msk;   // Specify that after an interrupt, the CPU wakes up

    __enable_interrupt();					// allow the CPU to respond to interrupt signals.
    NVIC->ISER[0] = 1 << ((WDT_A_IRQn) & 31); // enable WDT to send interrupt signals

    /*
     * A short 'forever' loop that we can look at in the debugger
     * which spends only a tiny time with the CPU active.
     */
    while (1)
    {
        /* Go to LPM0 mode (Low power mode with CPU powered off */
        __sleep();		  //
        __no_operation(); //  For debugger
    }
}



void mapports(){
	PMAP->KEYID=PMAP_KEYID_VAL; // unlock PMAP
	P2MAP->PMAP_REGISTER7=PMAP_TA0CCR0A;  // map TA0CCR0 to P2.7 as primary module
	PMAP->KEYID=0;				// relock PMAP until next hard reset
}


/*
 * Sound Production System
 *
 * In this example, we simply run TA0 in up mode
 * (TA0R goes from 0 to TA0CCR0 and then back to 0)
 * And when we want the sound on, we activate output mode 4 (toggle),
 * which makes the CCR0 output signal toggle each time TA0R hits TA0CCR0's value.
 * Thus, the half-period of the tone is TACCR0+1 timer ticks.
 *
 * The timer clock source is SMCLK (3Mhz), prescaled by 1, then divided by 3.
 * Thus it is about 1MHz.
 * Note: possible prescale factors are 1,2,4,8.
 *       possible additional EX0 dividers are 1,2,3,4,5,6,7,8
 *
 */

void init_timer(){              // initialization and start of timer
	TIMER_A0->CTL |= TIMER_A_CTL_CLR ;// reset clock
	TIMER_A0->CTL =  TIMER_A_CTL_TASSEL_2  // clock source = SMCLK
	                +TIMER_A_CTL_ID_0      // clock prescale=1
					+TIMER_A_CTL_MC_1;     // Up Mode
	TIMER_A0->EX0 = TIMER_A_EX0_TAIDEX_2;  // additional divisor=3

	TIMER_A0->CCTL[0]=TIMER_A_CCTLN_OUTMOD_4; // compare mode, output mode 4 (toggle)
	TIMER_A0->CCR[0] = initialHalfPeriod; // in up mode TAR=0... TACCRO-1
	P2->SEL0|=BIT7; // connect timer output to pin (select alternate function for pin)
	P2->DIR |=BIT7; // output mode on P2.7 (direction output completes setting the function)
}

/*
 * Button input System
 *
 * The Button toggles the state of the sound (on or off)
 * Action will be interrupt driven on a downgoing signal on the pin
 * no debouncing in this code, but MSP432P401R has glitch suppression which may help.
 */
