#include <SPI.h>
#include "ATM90E32.h"

/********************************************************/
/******************** Public Method *********************/
/********************************************************/

ATM90E32::ATM90E32() 
{
}

ATM90E32::~ATM90E32()
{
}

/* Inititalize ATM90E32 */
void ATM90E32::setup(unsigned int cs_pin)
{
  _cs_pin = cs_pin;

  pinMode(_cs_pin, OUTPUT);

  /* Enable SPI */
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE2);
  SPI.setClockDivider(SPI_CLOCK_DIV64);

  CommEnergyIC(0, SoftReset, 0x789A); //Perform soft reset
  CommEnergyIC(0, FuncEn0, 0x0030); //Voltage sag irq=1, report on warnout pin=1, energy dir change irq=0
  CommEnergyIC(0, FuncEn1, 0x0030); //Voltage sag irq=1, report on warnout pin=1, energy dir change irq=0
  CommEnergyIC(0, SagTh, 0x1F2F); //Voltage sag threshhold
  
  //Set metering config values
  CommEnergyIC(0, ConfigStart, 0x5678); //Metering calibration startup command. Register 31 to 3B need to be set
  CommEnergyIC(0, PLconstH, 0x00B9); //PL Constant MSB
  CommEnergyIC(0, PLconstL, 0xC1F3); //PL Constant LSB
  CommEnergyIC(0, MMode0, 0x0087); //Metering Mode Configuration. All defaults. See pg 58 of datasheet.
  CommEnergyIC(0, MMode1, 0x5555); //PGA Gain Configuration. x2 for DPGA and PGA. See pg 59 of datasheet
  CommEnergyIC(0, PStartTh, 0x08BD); //Active Startup Power Threshold
  CommEnergyIC(0, QStartTh, 0x0AEC); //Reactive Startup Power Threshold
  CommEnergyIC(0, CSZero, 0x5F59); //Write CSOne, as self calculated
  
  Serial.print("Checksum 0:");
  Serial.println(CommEnergyIC(1, CSZero, 0x0000), HEX); //Checksum 0. Needs to be calculated based off the above values.
  
  //Set metering calibration values
  CommEnergyIC(0, CalStart, 0x5678); //Metering calibration startup command. Register 41 to 4D need to be set
  CommEnergyIC(0, GainA, 0x1D39); //Line calibration gain
  CommEnergyIC(0, PhiA, 0x0000); //Line calibration angle
  CommEnergyIC(0, GainB, 0x1D39); //Line calibration gain
  CommEnergyIC(0, PhiB, 0x0000); //Line calibration angle
  CommEnergyIC(0, GainC, 0x1D39); //Line calibration gain
  CommEnergyIC(0, PhiC, 0x0000); //Line calibration angle
  CommEnergyIC(0, PoffsetA, 0x0000); //A line active power offset
  CommEnergyIC(0, QoffsetA, 0x0000); //A line reactive power offset
  CommEnergyIC(0, PoffsetB, 0x0000); //B line active power offset
  CommEnergyIC(0, QoffsetB, 0x0000); //B line reactive power offset
  CommEnergyIC(0, PoffsetC, 0x0000); //C line active power offset
  CommEnergyIC(0, QoffsetC, 0x0000); //C line reactive power offset
  CommEnergyIC(0, CSOne, 0x2402); //Write CSOne, as self calculated
  
  Serial.print("Checksum 1:");
  Serial.println(CommEnergyIC(1, CSOne, 0x0000), HEX); //Checksum 1. Needs to be calculated based off the above values.

  //Set measurement calibration values
  CommEnergyIC(0, AdjStart, 0x5678); //Measurement calibration startup command, registers 61-6F
  CommEnergyIC(0, UgainA, 0xD8E9);  //A SVoltage rms gain
  CommEnergyIC(0, IgainA, 0x1BC9); //A line current gain
  CommEnergyIC(0, UoffsetA, 0x0000); //A Voltage offset
  CommEnergyIC(0, IoffsetA, 0x0000); //A line current offset
  CommEnergyIC(0, UgainB, 0xD8E9);  //B Voltage rms gain
  CommEnergyIC(0, IgainB, 0x1BC9); //B line current gain
  CommEnergyIC(0, UoffsetB, 0x0000); //B Voltage offset
  CommEnergyIC(0, IoffsetB, 0x0000); //B line current offset
  CommEnergyIC(0, UgainC, 0xD8E9);  //C Voltage rms gain
  CommEnergyIC(0, IgainC, 0x1BC9); //C line current gain
  CommEnergyIC(0, UoffsetC, 0x0000); //C Voltage offset
  CommEnergyIC(0, IoffsetC, 0x0000); //C line current offset
  CommEnergyIC(0, CSThree, 0xA694); //Write CSThree, as self calculated

  Serial.print("Checksum 3:");
  Serial.println(CommEnergyIC(1, CSThree, 0x0000), HEX); //Checksum 3. Needs to be calculated based off the above values.

  CommEnergyIC(0, ConfigStart, 0x8765); //Checks correctness of 31-3B registers and starts normal metering if ok
  CommEnergyIC(0, CalStart, 0x8765); //Checks correctness of 41-4D registers and starts normal metering if ok
  CommEnergyIC(0, AdjStart, 0x8765); //Checks correct ness of 61-6F registers and starts normal measurement  if ok

  unsigned short systemstatus0 = CommEnergyIC(1, SysStatus0, 0xFFFF);

  if (systemstatus0 & 0x4000) {
    //checksum 1 error
    Serial.println("checksum error 0");
  }
  if (systemstatus0 & 0x1000) {
    //checksum 2 error
    Serial.println("checksum error 1");
  }
  if (systemstatus0 & 0x0400) {
    //checksum 2 error
    Serial.println("checksum error 2");
  }
  if (systemstatus0 & 0x0100) {
    //checksum 2 error
    Serial.println("checksum error 3");
  }
}

/* Read all data from ATM90E32 device */
void ATM90E32::handle(void)
{
  _line_A.voltage = CommEnergyIC(1, UrmsA, 0xFFFF) / 100.f;
  _line_B.voltage = CommEnergyIC(1, UrmsB, 0xFFFF) / 100.f;
  _line_C.voltage = CommEnergyIC(1, UrmsC, 0xFFFF) / 100.f;

  _line_A.current = CommEnergyIC(1, IrmsA, 0xFFFF) / 1000.f;
  _line_B.current = CommEnergyIC(1, IrmsB, 0xFFFF) / 1000.f;
  _line_C.current = CommEnergyIC(1, IrmsC, 0xFFFF) / 1000.f;

  _line_A.power = CommEnergyIC(1, PmeanA, 0xFFFF) / 10.f;
  _line_B.power = CommEnergyIC(1, PmeanB, 0xFFFF) / 10.f;
  _line_C.power = CommEnergyIC(1, PmeanC, 0xFFFF) / 10.f;

  _frequency = CommEnergyIC(1, Freq, 0xFFFF) / 100.f;
}

/********************************************************/
/******************** Private Method ********************/
/********************************************************/

/* Communication with ATM90E32 */
unsigned short ATM90E32::CommEnergyIC(unsigned char RW, unsigned short address, unsigned short val) 
{
  unsigned char* data = (unsigned char*)&val;
  unsigned char* adata = (unsigned char*)&address;
  unsigned short output;
  unsigned short address1;

  //switch MSB and LSB of value
  output = (val >> 8) | (val << 8);
  val = output;

  //Set read write flag
  address |= RW << 15;
  //Swap address bytes
  address1 = (address >> 8) | (address << 8);
  address = address1;

  //enable chip and wait for SPI bus to activate
  digitalWrite (_cs_pin, LOW);
  delayMicroseconds(10);
  //Write address byte by byte
  for (byte i=0; i<2; i++) {
    SPI.transfer (*adata);
    adata++;
  }
  /* Must wait 4 us for data to become valid */
  delayMicroseconds(4);

  //Read data
  //Do for each byte in transfer
  if (RW) {
	  for (byte i=0; i<2; i++) {
      *data = SPI.transfer(0x00);
      data++;
    }
  }
  else {
	  for (byte i=0; i<2; i++) {
      SPI.transfer(*data);  // write all the bytes
      data++;
    }
  }

  delayMicroseconds(10);
  digitalWrite(_cs_pin, HIGH);

  output = (val >> 8) | (val << 8); //reverse MSB and LSB
  return output;
}

#if !defined(NO_GLOBAL_INSTANCES) 
ATM90E32 Monitoring;
#endif
