/*
	This block of code communicates with a salvaged Xbox 360 RF module via a 10 bit serial protocol to provide visual feedback and sync the module with external controllers.
	It is designed to be run on an ATTiny25 clocked at about 1MHz (using the defailt RC oscilator and clock divider settings). This source code is provided free for any purpose, personal or otherwise.

	To compile and burn:
		avr-gcc -g -Wall -O2 -mmcu=attiny25 -c -o xboxrf.o xboxrf.c
		avr-gcc -g -Wall -O2 -mmcu=attiny25 -o xboxrf.elf xboxrf.o 
		avr-objcopy -j .text -j .data -O ihex xboxrf.elf xboxrf.hex
		sudo avrdude -c usbtiny -p t25 -U flash:w:xboxrf.hex:i

*/

#define F_CPU 1000000	//1MHz, needed for delay function

#include <inttypes.h>
#include <avr/io.h>
//#include <avr/interrupt.h>
//#include <avr/sleep.h>
#include <util/delay.h>

//There is quite a number of functions available for this board, however we only need a few for basic operation.
#define XBOX_SYNC 		0x004	//Tell the RF module to sync with external controllers, and play the sync animation on the LEDs
#define XBOX_BOOTANIM	0x085	//Initalize the LEDs and play the Xbox 360 'power on' animation on the LEDs. An 'init' function (which this instruction is) must be run before the LEDs can be used.
#define XBOX_GREEN_ALL	0x0AF	//Turn on all green LEDs

//This function sends the lower ten bits of a 16 bit value to the RF module
//Bits are clocked out MSB first
void sendword(uint16_t werd) {
	PORTB &= ~_BV(PB3);		//Bring data line low

	uint8_t index = 0;

	while (index < 10) {
		while ((PINB & _BV(PB4)) != 0) {;}		//Wait for clock line to go low

		//Figure out what the next bit in the data word is, and send it to the output pin
		if ((werd & (0x200 >> index)) == 0) {
			PORTB &= ~_BV(PB3);		//Bring data line low
		} else {
			PORTB |= _BV(PB3);		//Bring data line high
		}

		while ((PINB & _BV(PB4)) == 0) {;}		//Wait for clock line to go high

		index++;
	}

	PORTB |= _BV(PB3);	//Bring data line high

	//High fives all around
}


int main (void)
{
	/*Okay, we have a few pin directions to set:
		PB0 - Input, pullups enabled. Xbox 360 power button, pulls line low when pressed. 
		PB3 - Output. Serial Data line.
		PB4 - Input, no pullups. Serial clock from RF module.
	*/
	DDRB  = _BV(PB3);				//Only PB3 is an output
	PORTB = _BV(PB3) | _BV(PB0);	//Enable the pull-ups on PB0 and bring the ouput line high (it's default state)

	_delay_ms (3000);	//Give the RF module a moment to boot up before we start slinging data at it.

	sendword(XBOX_BOOTANIM);
	//_delay_ms (8000);
	//sendword(XBOX_GREEN_ALL);

	//This isn't the best way to do it, but it's quick and easy to implement
	for(;;) {

		//Check to see if PB0 - the power button line - is low. If it is, the user has pressed it.
		if ((PINB & _BV(PB0)) == 0) {
			sendword(XBOX_SYNC);	//Tell the RF module to sync with controllers!
			_delay_ms (5000);		//Diable the sync button for a couple of seconds
		}

	}


    return (0);
}
