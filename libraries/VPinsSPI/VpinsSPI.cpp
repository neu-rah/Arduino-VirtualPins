#include <Arduino.h>
#include <virtual_pins.h>
#include "VPinsSPI.h"
#include "../SPI/SPI.h"

#define PORTREGSZ 3//why is this not defined! damn weird build!

//give real pin for spi latch, virtual port number, and # of ports
SPIBranch::SPIBranch(SPIClass &spi,char latch_pin,char port,char sz):SPI(spi),latchPin(latch_pin),portBranch(port,sz),ioMode(VPSPI_COMPAT) {
	Serial.print("SPIBranch");
	pinMode(latchPin,OUTPUT);
	on(latchPin);
	//SPI.begin();
	//user can define latch initial status (we will just toggle it)
	//this will select positive/negative polarity of the latch pin
	//for the hw spi SPI.setDataMode(...) can be used
	//also spi clock can be adjusted
}

void SPIBranch::mode() {}//this is internal control no meaning on the target shift registers
void SPIBranch::in() {io();}//call io because SPI bus is full-duplex
void SPIBranch::out() {io();}//call io because SPI bus is full-duplex

//do input and output (SPI is a bidirectional bus)
void SPIBranch::io() {
	Serial.print("SPIBranch::io");
	pulse(latchPin);//read data (will also show output data)
	switch(ioMode) {
 	case VPSPI_COMPAT: {
			//in this mode pins can be input or output but not both at same time (still read all at once)
			//if the pin is in output mode, reading data will read the outputed data
			//if pin is input, setting output will do nothing unless we have an external pull resistor to the input
			int lastPort=localPort+size-VPA-1;
			for(int p=lastPort;p>lastPort-size;p--) {
				vpins_data[PORTREGSZ*(size-p-1)+2]=
					(SPI.transfer(vpins_data[PORTREGSZ*p+1]) & ~vpins_data[PORTREGSZ*(size-p-1)])
					| (vpins_data[PORTREGSZ*(size-p-1)+1] & vpins_data[PORTREGSZ*(size-p-1)]);
			}
		}
		break;
	case VPSPI_DUPLEX:
		// in this mode: separate inputs and outputs (still read all at once), only keeps data apart
		// even with separate data and working independent (in/out) pins will have the same number
		// digitalWrite(20,x) will affect 1st data pin of first shiftout register
		// digitalread(20) will get data from 1st input pin of first shiftin register
	default:
		int lastPort=localPort+size-VPA-1;
		for(int p=lastPort;p>lastPort-size;p--) {
			vpins_data[PORTREGSZ*(size-p-1)+2]=SPI.transfer(vpins_data[PORTREGSZ*p+1]);
		}
		break;
	}
	pulse(latchPin);//write data
}


