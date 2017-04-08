#ifndef SPI_VPINS_PROTOCOL_DEF
#define SPI_VPINS_PROTOCOL_DEF
	#include <virtual_pins.h>
	#include <SPI.h>
	
	#define VPSPI_COMPAT 0
	#define VPSPI_DUPLEX	1

	//SPI hardware port
	class SPIBranch:public portBranch {//wil handle SPI comunication
	private:
		char ioMode;
		SPIClass& SPI;
	public:
		char latchPin;//aux pin to kick data in/out
		//SPIBranch(char latch_pin,char port,char sz);
		SPIBranch(SPIClass &spi,char latch_pin,char port,char sz);
		void setVPinsIO(int mode);
		inline void compatMode() {ioMode=VPSPI_COMPAT;}
		inline void duplexMode() {ioMode=VPSPI_DUPLEX;}
		virtual void mode();
		virtual void in();
		virtual void out();
		virtual void io();
	};
#endif
