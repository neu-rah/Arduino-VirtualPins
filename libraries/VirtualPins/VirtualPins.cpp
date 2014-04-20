#include <virtual_pins.h>
#include "VirtualPins.h"
#include <Arduino.h>
#include "../SPI/SPI.h"
#include "../Wire/Wire.h"

//give real pin for spi latch, virtual port number, and # of ports
SPIBranch::SPIBranch(SPIClass &spi,char latch_pin,char port,char sz):SPI(spi),latchPin(latch_pin),portBranch(port,sz),ioMode(VPSPI_COMPAT) {
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////
I2CBranch::I2CBranch(TwoWire & wire,char id,char local,char sz)
	:Wire(wire),serverId(id),portBranch(local,sz) {
}

void I2CBranch::io() {in();out();}
void I2CBranch::mode() {}//TODO: see PCA9557, derive the class to support specific hardware or family
void I2CBranch::in() {}//TODO: test i2c input shift registers... have none till now

void I2CBranch::out() {
  Wire.beginTransmission(serverId);
  for(int n=localPort;n<localPort+size;n++)
    while (Wire.write(*portOutputRegister(localPort))!=1);
  Wire.endTransmission(serverId);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
I2CServerBranch::I2CServerBranch(TwoWire & wire,char id,char local,char host,char sz):hostPort(host),I2CBranch(wire,id,local,sz) {
	//TODO: wait for server to be ready
	//TODO: need a timeout and an error status somewhere
	/*do Wire.beginTransmission(id);
	while(!Wire.endTransmission());*/
}

void I2CServerBranch::mode() {dispatch(0b00);}
void I2CServerBranch::in() {
	char op=0b10;
  Wire.beginTransmission(serverId);
  Wire.write((hostPort<<2)|op);//codify operation on lower 2 bits
	int nbytes=Wire.requestFrom(serverId, 1);
  	*(portInputRegister(localPort))=Wire.read();
  Wire.endTransmission(serverId);
}
void I2CServerBranch::out() {dispatch(0b01);}

//op is port data info (3 bytes) index, avr ports compatible
void I2CServerBranch::dispatch(char op) {
	Serial.println("Wire dispatch");
  Wire.beginTransmission(serverId);
  Wire.write((hostPort<<2)|op);//codify operation on lower 2 bits
  for(int n=0;n<size;n++) {
  	Wire.write(*(portModeRegister(localPort+n)+op));
  }
  Wire.endTransmission(serverId);
}


