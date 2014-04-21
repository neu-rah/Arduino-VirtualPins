/*
Virtual pins library
  LiquidCrystal classic example  - "Hello World" using virtual pins
*/

#include <SPI.h>// <---<< we will use this media
#include <VPinsSPI.h>

// include the library code:
#include <LiquidCrystal.h>

#define STCP 9//stcp or latch pin
SPIBranch spi(SPI,STCP,VPA,2);//TODO: add version that finds a free port and make pin() fynctions the only methid to get a pin number

// initialize the library with the numbers of the interface pins
//LiquidCrystal lcd(20, 21, 22, 23, 24, 25);// <-----<<< using virtual pins number! must update if changed port
//LiquidCrystal lcd(vpA(0), vpA(1), vpA(2), vpA(3), vpA(4), vpA(5));// <-----<<< using virtual pins macros!, also must update if changing port
LiquidCrystal lcd(spi.pin(0), spi.pin(1), spi.pin(2), spi.pin(3), spi.pin(4), spi.pin(5));// <-----<<< using virtual pins function! auto updates on port change

void setup() {
  SPI.begin();//<<-- initialize the media
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("hello, world!");
}

void loop() {
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 1);
  // print the number of seconds since reset:
  lcd.print(millis()/1000);
}

