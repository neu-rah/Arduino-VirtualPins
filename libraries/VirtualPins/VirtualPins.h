#ifndef SPI_VPINS_PROTOCOL_DEF
#define SPI_VPINS_PROTOCOL_DEF

	#include <SPI.h>

	#define VPSPI_COMPAT 0
	#define VPSPI_DUPLEX	1

	//SPI hardware port
	class SPIBranch:public portBranch {//wil handle SPI comunication
	private:
		char ioMode;
		SPIClass SPI;
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

	//I2C hardware port
	class I2CShiftRegBranch:public portBranch {
	public:
		char serverId;//l1ike i2c server
		I2CShiftRegBranch(char id,char local,char sz=1);
		virtual void mode();
		virtual void in();
		virtual void out();
		virtual void io();
	};

	//virtual port over I2c (target can be any hardware or virtual port at server)
	class I2CBranch:public I2CShiftRegBranch {
	private:
		void dispatch(char op);
	public:
		char hostPort;//host port nr
		I2CBranch(char id,char local,char host,char sz=1);
		virtual void mode();
		virtual void in();
		virtual void out();
	};
#endif
