/**************************

  Copyright (c) 2014 Rui Azevedo

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General
  Public License along with this library; if not, write to the
  Free Software Foundation, Inc., 59 Temple Place, Suite 330,
  Boston, MA  02111-1307  USA

**********************
Virtual Pins Extension
virtualize device connection to avoid library change due to change in connecting media.

(concept:
- Use hardware that can usually be direct-connect to the AVR as a "network" device
- Be transparent as network printers are on an OS, use the same driver and make the OS redirect the data to the network
- we should have the same library to use either with direct connected devices or hosted on shift registers no mather if they are SPI or I2C or whatever.
	
(resume:
when using devices thru shift registers there is a need to change the original library to use the new connecting media, sutch is the case of the standard Liquidcrystal library.
This process is contrary of library purpose, produces a proliferations of versions and mantaining the original library by update will generate a nightmare of variations update and/or abandon.
To solve this problem we have created the concept of virtual pins in the arduino environment. This is not a library because it must integrate with system at low level.
Some virtual ports are created (8 pins each) and visible to the environmente as extra pin numbers, the virtual ports can be directed to I2C or SPI hardware (as shift registers) and be used to initialize the library object giving the new available pin numbers. On arduino Uno the start at pin 20 and make them transparent to other libraries/applications (with timming restrictions ofcoz) without need of change the libraries or code.

(limitations:
- the limitations reflect the nature of the connected hardware.
- processors lacking memory access interrupts => low level port operations should be complemented with register update calls
- simple IO pins, special hardware functionality of pins like PWM, timmer/count, pin change interrupt are not avalable. Unless of course the hardware supplyes them
- trading pins by time, things on virtual pins will run slower. But speed is not the purpose of using shift-registers.

(definition:
following the same arduino line of port mapping (trading processing time by memory) we are using some predefined PROGMEM ammount (extendind arduino ide pin maps), using pinmaps limits the available pins numbers, but does not inpact performance of native pin operations the possibility of having functions called when mapping a pin to port would be too mutch processor expensive.
so:
- VPins are coded to fit the lowest level possible. 
		(this processor has no memory access interrupt, therefore not allowing a full implementation)
	digitalPinToBitMask, digitalPinToPort, portOutputRegister and similar functions will stil work
	each virtual port will have compatible DDR PORT PIN chars.
	however iteracting at this level requieres a call to the dispatch functions (no mem access interrupt)
	either before (when reading) of after (when writing), or both if performing input/output (SPI supports full duples)
- To reach this level it was coded as an extension to arduino IDE environment and not as a library
- pinMode, digitalRead, digitalWrite will work the same way
	so working at this level everything is transparent
	you can use an lcd like:
		LiquidCrystal lcd(20, 21, 22, 23, 24, 25)
		...
		lcd.begin(16,2);
		lcd.print("Hello world");
		lcd.setCursor(0,1);
		...
	apart the unusual pin numbers, no modifications are required
	the hardware should respond slower, but its more than enough for that lcd and many devices
	
- Extensible, the base system just does the pins allocation and provides no comunication code, protocols for SPI or I2C are added as libraries and can be extend to support other comunication midia.
As a base concept "virtual pins" dispatch port data to a network port (SPI, I2C, Serial, etc..) and expect
a server/slave to be present at the other side to fullfill the request
in the case of SPI the servers/slaves can be simple shift registers (either input or output)
so virtual pins are a network client/requester.

COMPAT MODE makes the pin work only in one direction at a time
- can read input from the output pins, resulting on whatever state the last output left them
TODO: fix some bugs there! masking the data
- this mode is slower, however it reflects the operation mode of native pins
	and allow thigs like:
		digitalWrite(x,!digitalRead(x));//<--- pin toggle
	to still work.
-shift registers arrays can coexist as shift-in and shift-out chain of independent length
	4 virttual ports =>	4 shift-out + 4 shift-in => 32 IO pins

DUPLEX MODE makes the IO data of the pin mode-independent
-this mode allows the same pin number to work as input and output at the same time
	(because they are in reality 2 diferent physical pins (optionally omited one)
-shift registers arrays can coexist as shift-in and shift-out chain of independent length
	4 virttual ports =>	4 shift-out + 4 shift-in => 32 input pins + 32 output pins => 64 extra pins

Addiing pull resistor in VPIO_COMPAT mode (can someone test this?)
	pull up resistor can be activated (using diode and resistor serie from output to input)
	SHIFT OUT pin x]o---47k--->|---o[SHIFT IN pin x

	resistor can pull up/down (just omit the diode)
	SHIFT OUT pin x]o---47k---o[SHIFT IN pin x

tested with:
	LS74HC595 <-- outputs
	LS74HC165N <-- inputs

you should NOT use a virtual pin for:
	- controlling the SPI latch of the same SPI port, this pin needs to kick data IO on the SPI
	- timmed signals, virtual pins dont have an acurate timming, however you can blik led with them
	- pin change interrupt (never seen a shift register with and interrupt, so...)
		we might consider it for i2c and generalize, still wont be an accurate timming pin
	-	real time critical control (IO)
	- internal pull-up resistor (unless you wire an external one, this pins have no resistor)
	
My wish list:
	- atmel should include this virtual maps allong with the physical port registers (dont they? why? its reserved... for this?)
	- at least this maps should have an interrupt vector to be called on access (=> memory access look mechanisms)
	- can we have circuits of atmel pins with SPI interface (supper shift registers)
	- next i have to code the VPins server (this can be a library) and will explode the functionality
		- can have pin Server and not be a client
		- can have both pins server and client (do we need routers?, lol... because we have them)
	- after that i want I2C atmel pin chips :D
	- allong the way someone should make an apache client extension to run on a raspi and let the raspi use the devices thru the I2C bus... neat!

ruihfazevedo@gmail.com 
Feb.2014

example: wiring 74hc595 shift register to arduino SPI pins
using avr hardware SPI

  74HC595 <-> ATMega328P
================================
					 -	CS (|10|)
(11) SHCP <-> SCK (19|13|B5)
(14)   DS <-> MOSI (17|11|B3)
(12) STCP <-> (choose your pin, 10 is no good, its active... its nice to AND and mask CE|SS for multi-slave modes)
**********************/

#ifdef USE_VIRTUAL_PINS
	#ifndef VIRTUAL_PINS_DEF

		#define VIRTUAL_PINS_DEF
		//number of 8bit ports to use
		#define VPINS_PORTS 4
		//allocated memory size (in bytes)
		#define PORTREGSZ 3
		#define VPINS_SZ (VPINS_PORTS*PORTREGSZ)//we are using 3 bytes per port
		extern char vpins_data[VPINS_SZ];//and this is the memory for it (12 bytes= 32in+32out)
		//max number of protocol stacks
		#define branchLimit 8
		#define NOT_A_BRANCH -1

		#define DDR_VPA (vpins_data+0)
		#define PORT_VPA (vpins_data+1)
		#define PIN_VPA (vpins_data+2)
		#define DDR_VPB (vpins_data+3)
		#define PORT_VPB (vpins_data+4)
		#define PIN_VPB (vpins_data+5)
		#define DDR_VPC (vpins_data+6)
		#define PORT_VPC (vpins_data+7)
		#define PIN_VPC (vpins_data+8)
		#define DDR_VPD (vpins_data+9)
		#define PORT_VPD (vpins_data+10)
		#define PIN_VPD (vpins_data+11)

		#define VP0 20
		#define VP1 21
		#define VP2 22
		#define VP3 23
		#define VP4 24
		#define VP5 25
		#define VP6 26
		#define VP7 27
	
		#define VP8 28
		#define VP9 29
		#define VP10 30
		#define VP11 31
		#define VP12 31
		#define VP13 33
		#define VP14 34
		#define VP15 35
	
		#define VP16 36
		#define VP17 37
		#define VP18 38
		#define VP19 39
		#define VP20 40
		#define VP21 41
		#define VP22 42
		#define VP23 43
	
		#define VP24 44
		#define VP25 45
		#define VP26 46
		#define VP27 47
		#define VP28 48
		#define VP29 49
		#define VP30 50
		#define VP31 51
	
		#define VPA 13
		#define VPB 14
		#define VPC 15
		#define VPD 16

		//utility macros
		#define on(x) digitalWrite(x,1)
		#define off(x) digitalWrite(x,0)
		#define tog(x) digitalWrite(x,!digitalRead(x))
		#define pulse(x) {tog(x);tog(x);}
		#define NATIVE_PORTS digitalPinToPort(NUM_DIGITAL_PINS)
		#define NATIVE_PORT(x) (x<=digitalPinToPort(NUM_DIGITAL_PINS)))

		//tease virtual pin numbers by using virtual ports
		//virtual pin 35 = VP15 = VP(VPA,15) = VP(VPB,7) = vpA(15) = vpB(7)
		#define VP(port,pin) (NUM_DIGITAL_PINS+(port-VPA)*8+pin)
		#define vpA(pin) (VP(VPA,pin))
		#define vpB(pin) (VP(VPB,pin))
		#define vpC(pin) (VP(VPC,pin))
		#define vpD(pin) (VP(VPD,pin))

		//#ifdef USE_VIRTUAL_PINS
			#ifdef __cplusplus
			extern "C" {
			#endif
			//void vpins_begin(char lp,char regsz);
			void vpins_begin();
			void vpins_mode(char port);
			void vpins_in(char port);
			void vpins_out(char port);
			void vpins_io(char port);//use portmap to dispatch network port (includes SPI)
			#ifdef __cplusplus
			}
			#endif
		//#endif

	#endif

	#ifdef __cplusplus
		#ifndef VIRTUAL_PINS_CPP_DEF
			#define VIRTUAL_PINS_CPP_DEF

			#define NOBRANCH ((portBranch*)0)
			class portBranch;
			extern portBranch* tree[branchLimit];
			extern portBranch* debug_branch;
			#ifdef VPINS_OPTIMIZE_SPEED
				extern char port_to_branch[];
			#endif
			
			class portBranch {
			private:
			public:
				char index;
				bool active;//branch mounted ok?
				char size;//number of ports on this chain (must be sequential)
				char localPort;//local port nr, it can be a vrtual port :D
				portBranch(char port, char sz);
				//portBranch(char sz);
				virtual ~portBranch();
				inline bool hasPort(char port) {return port>=localPort && port<(localPort+size);}
				inline int pin(int p) {return 20+(index<<3)+p;}//TODO: NUM_DIGITAL_PINS not available here?!!!
				/*static inline int freePort() {
					for(int i=0;i<ports_limit;i++) if (port_to_Branch[i]}*/
				static char getBranchId(char port);
				static portBranch& getBranch(char port);
				//this functions kick data in/out of the virtual ports
				//on SPI (and duplex protocols) io is always called
				//on ither protocols we have advantage of calling either in or out
				//port mode is only sent out for network ports
				//default branch type does nothing (ideal for representing reah hardware ports)
				virtual void mode();
				virtual void in();
				virtual void out();
				virtual void io();
			};

		#endif
	#endif
#endif
