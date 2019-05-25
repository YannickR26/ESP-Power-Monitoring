#include <SPI.h>
#include "ATM90E32.h"

#include "JsonConfiguration.h"

/********************************************************/
/******************** Public Method *********************/
/********************************************************/

ATM90E32::ATM90E32() 
{
  _lgain = 100;
  _ugain = 1000;
  _igain = 100;
  resetAllConso();
}

ATM90E32::~ATM90E32()
{
}

/* Inititalize ATM90E32 */
void ATM90E32::setup(int cs_pin, int pm0_pin, int pm1_pin)
{
  _cs_pin = cs_pin;
  _pm0_pin = pm0_pin;
  _pm1_pin = pm1_pin;

  pinMode(_cs_pin, OUTPUT);
  // set to Normal mode
  if (_pm0_pin != -1) {
    pinMode(_pm0_pin, OUTPUT);
    digitalWrite(_pm0_pin, HIGH);
  }
  if (_pm1_pin != -1) {
    pinMode(_pm1_pin, OUTPUT);
    digitalWrite(_pm1_pin, HIGH);
  }

  delayMicroseconds(1000);

  /* Enable SPI */
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE2);
  SPI.setClockDivider(SPI_CLOCK_DIV128);

  CommEnergyIC(WRITE, SoftReset, 0x789A); //Perform soft reset
  CommEnergyIC(WRITE, CfgRegAccEn, 0x55AA);	// Enable register config access
  CommEnergyIC(WRITE, MeterEn, 0x0001); 		// Enable Metering
	
	/* SagTh = Vth * 100 * sqrt(2) / (2 * Ugain / 32768) */
  CommEnergyIC(WRITE, SagTh, 0x1000);       // Voltage sag threshold
  CommEnergyIC(WRITE, FreqHiTh, 0x13EC);	// High frequency threshold - 51.00Hz (default)
  CommEnergyIC(WRITE, FreqLoTh, 0x1324);	// Lo frequency threshold - 49.00Hz (default)
  CommEnergyIC(WRITE, EMMIntEn0, 0x0000);	// Disable interrupts
  CommEnergyIC(WRITE, EMMIntEn1, 0x0000);	// Disable interrupts
  CommEnergyIC(WRITE, EMMIntState0, 0x0001);	// Clear interrupt flags
  CommEnergyIC(WRITE, EMMIntState1, 0x0001);	// Clear interrupt flags
  CommEnergyIC(WRITE, ZXConfig, 0x0001);	// disable all the ZX signals to ‘0’
  
  //Set metering config values (CONFIG)
  CommEnergyIC(WRITE, PLconstH, 0x0861);    // PL Constant MSB (default)
  CommEnergyIC(WRITE, PLconstL, 0x4C68);    // PL Constant LSB (default)
  if (Configuration._mode == 0) {
    CommEnergyIC(WRITE, MMode0, 0x0087);      // Mode Config (50 Hz, 3P4W) (default)
  }
  else {
    CommEnergyIC(WRITE, MMode0, 0x0185);      // Mode Config (50 Hz, 3P3W, phase B not counted)
  }
  CommEnergyIC(WRITE, MMode1, 0x0000);      // PGA Gain Config - 0x5555 (x2) // 0x0000 (1x) (default)
  CommEnergyIC(WRITE, PStartTh, 0x0000);    // Active Startup Power Threshold
  CommEnergyIC(WRITE, QStartTh, 0x0000);    // Reactive Startup Power Threshold
  CommEnergyIC(WRITE, SStartTh, 0x0000);    // Apparent Startup Power Threshold
  CommEnergyIC(WRITE, PPhaseTh, 0x0000);    // Active Phase Threshold
  CommEnergyIC(WRITE, QPhaseTh, 0x0000);    // Reactive Phase Threshold
  CommEnergyIC(WRITE, SPhaseTh, 0x0000);    // Apparent  Phase Threshold
  
  //Set metering calibration values (CALIBRATION)
  CommEnergyIC(WRITE, GainA, _lgain);       // Line calibration gain
  CommEnergyIC(WRITE, PhiA, 0x0000);        // Line calibration angle
  CommEnergyIC(WRITE, GainB, _lgain);       // Line calibration gain
  CommEnergyIC(WRITE, PhiB, 0x0000);        // Line calibration angle
  CommEnergyIC(WRITE, GainC, _lgain);       // Line calibration gain
  CommEnergyIC(WRITE, PhiC, 0x0000);        // Line calibration angle
  CommEnergyIC(WRITE, PoffsetA, 0x0000);    // A line active power offset
  CommEnergyIC(WRITE, QoffsetA, 0x0000);    // A line reactive power offset
  CommEnergyIC(WRITE, PoffsetB, 0x0000);    // B line active power offset
  CommEnergyIC(WRITE, QoffsetB, 0x0000);    // B line reactive power offset
  CommEnergyIC(WRITE, PoffsetC, 0x0000);    // C line active power offset
  CommEnergyIC(WRITE, QoffsetC, 0x0000);    // C line reactive power offset
  
  //Set metering calibration values (HARMONIC)
  CommEnergyIC(WRITE, POffsetAF, 0x0000);   // A Fund. active power offset
  CommEnergyIC(WRITE, POffsetBF, 0x0000);   // B Fund. active power offset
  CommEnergyIC(WRITE, POffsetCF, 0x0000);   // C Fund. active power offset
  CommEnergyIC(WRITE, PGainAF, 0x0000);     // A Fund. active power gain
  CommEnergyIC(WRITE, PGainBF, 0x0000);     // B Fund. active power gain
  CommEnergyIC(WRITE, PGainCF, 0x0000);     // C Fund. active power gain

  //Set measurement calibration values (ADJUST)
  CommEnergyIC(WRITE, UgainA, _ugain);      // A Voltage rms gain
  CommEnergyIC(WRITE, IgainA, _igain);      // A line current gain
  CommEnergyIC(WRITE, UoffsetA, 0x0000);    // A Voltage offset
  CommEnergyIC(WRITE, IoffsetA, 0x0000);    // A line current offset
  CommEnergyIC(WRITE, UgainB, _ugain);      // B Voltage rms gain
  CommEnergyIC(WRITE, IgainB, _igain);      // B line current gain
  CommEnergyIC(WRITE, UoffsetB, 0x0000);    // B Voltage offset
  CommEnergyIC(WRITE, IoffsetB, 0x0000);    // B line current offset
  CommEnergyIC(WRITE, UgainC, _ugain);      // C Voltage rms gain
  CommEnergyIC(WRITE, IgainC, _igain);      // C line current gain
  CommEnergyIC(WRITE, UoffsetC, 0x0000);    // C Voltage offset
  CommEnergyIC(WRITE, IoffsetC, 0x0000);    // C line current offset

  CommEnergyIC(WRITE, CfgRegAccEn, 0x0000);	// End configuration
}

/* Read all data from ATM90E32 device */
void ATM90E32::handle(void)
{
  static unsigned long oldTick;
  unsigned long time = (millis() - oldTick) / 1000;
  oldTick = millis();

  // unsigned short sys0 = GetSysStatus0();
  // unsigned short sys1 = GetSysStatus1();
  // unsigned short en0 = GetMeterStatus0();
  // unsigned short en1 = GetMeterStatus1();

  // Serial.println("Sys Status: S0:0x" + String(sys0, HEX) + " S1:0x" + String(sys1, HEX));
  // Serial.println("Meter Status: E0:0x" + String(en0, HEX) + " E1:0x" + String(en1, HEX));

  _line_A.voltage = GetLineVoltageA();
  _line_B.voltage = GetLineVoltageB();
  _line_C.voltage = GetLineVoltageC();

  _line_A.current = GetLineCurrentA();
  _line_B.current = GetLineCurrentB();
  _line_C.current = GetLineCurrentC();

  _line_A.power = GetActivePowerA();
  _line_B.power = GetActivePowerB();
  _line_C.power = GetActivePowerC();

  // _line_A.conso += (_line_A.power / 3600) * time;
  // _line_B.conso += (_line_B.power / 3600) * time;
  // _line_C.conso += (_line_C.power / 3600) * time;

  _frequency = GetFrequency();

  // Serial.printf("Temp: %.1f°C\n", GetTemperature());
  
  Serial.printf("Line A =>, %.1fV, %.2fA, %.1fW, %.1fHz\n", _line_A.voltage, _line_A.current, _line_A.power, _frequency);
  Serial.printf("Line B =>, %.1fV, %.2fA, %.1fW, %.1fHz\n", _line_B.voltage, _line_B.current, _line_B.power, _frequency);
  Serial.printf("Line C =>, %.1fV, %.2fA, %.1fW, %.1fHz\n", _line_C.voltage, _line_C.current, _line_C.power, _frequency);
}

/********************************************************/
/******************** Private Method ********************/
/********************************************************/

// VOLTAGE
double  ATM90E32::GetLineVoltageA() {
  unsigned short voltage = CommEnergyIC(READ, UrmsA, 0xFFFF);
  return (double)voltage / 100;
}

double  ATM90E32::GetLineVoltageB() {
  unsigned short voltage = CommEnergyIC(READ, UrmsB, 0xFFFF);
  return (double)voltage / 100;
}

double  ATM90E32::GetLineVoltageC() {
  unsigned short voltage = CommEnergyIC(READ, UrmsC, 0xFFFF);
  return (double)voltage / 100;
}

// CURRENT
double ATM90E32::GetLineCurrentA() {
  unsigned short current = CommEnergyIC(READ, IrmsA, 0xFFFF);
  return (double)current / 1000;
}
double ATM90E32::GetLineCurrentB() {
  unsigned short current = CommEnergyIC(READ, IrmsB, 0xFFFF);
  return (double)current / 1000;
}
double ATM90E32::GetLineCurrentC() {
  unsigned short current = CommEnergyIC(READ, IrmsC, 0xFFFF);
  return (double)current / 1000;
}

// ACTIVE POWER
double ATM90E32::GetActivePowerA() {
  signed short apower = (signed short) CommEnergyIC(READ, PmeanA, 0xFFFF); 
  if (apower & 0x8000) {
    apower= (apower & 0x7FFF) * -1;
  }
  return (double)apower / 1000;
}
double ATM90E32::GetActivePowerB() {
  signed short apower = (signed short) CommEnergyIC(READ, PmeanB, 0xFFFF); 
  if (apower & 0x8000) {
    apower= (apower & 0x7FFF) * -1;
  }
  return (double)apower / 1000;
}
double ATM90E32::GetActivePowerC() {
  signed short apower = (signed short) CommEnergyIC(READ, PmeanC, 0xFFFF); 
  if (apower & 0x8000) {
    apower= (apower & 0x7FFF) * -1;
  }
  return (double)apower / 1000;
}
double ATM90E32::GetTotalActivePower() {
  signed short apower = (signed short) CommEnergyIC(READ, PmeanT, 0xFFFF); 
  if (apower & 0x8000) {
    apower= (apower & 0x7FFF) * -1;
  }
  return (double)apower / 1000; //240
}

// REACTIVE POWER
double ATM90E32::GetReactivePowerA() {
  signed short apower = (signed short) CommEnergyIC(READ, QmeanA, 0xFFFF); 
  if (apower & 0x8000) {
    apower= (apower & 0x7FFF) * -1;
  }
  return (double)apower / 1000;
}
double ATM90E32::GetReactivePowerB() {
  signed short apower = (signed short) CommEnergyIC(READ, QmeanB, 0xFFFF); 
  if (apower & 0x8000) {
    apower= (apower & 0x7FFF) * -1;
  }
  return (double)apower / 1000;
}
double ATM90E32::GetReactivePowerC() {
  signed short apower = (signed short) CommEnergyIC(READ, QmeanC, 0xFFFF);
  if (apower & 0x8000) {
    apower= (apower & 0x7FFF) * -1;
  }
  return (double)apower / 1000;
}
double ATM90E32::GetTotalReactivePower() {
  signed short apower = (signed short) CommEnergyIC(READ, QmeanT, 0xFFFF); 
  if (apower & 0x8000) {
    apower= (apower & 0x7FFF) * -1;
  }
  return (double)apower / 1000; //250
}

// APPARENT POWER
double ATM90E32::GetApparentPowerA() {
  signed short apower = (signed short) CommEnergyIC(READ, SmeanA, 0xFFFF); 
  if (apower & 0x8000) {
    apower= (apower & 0x7FFF) * -1;
  }
  return (double)apower / 1000;
}
double ATM90E32::GetApparentPowerB() {
 signed short apower = (signed short) CommEnergyIC(READ, SmeanB, 0xFFFF);
  if (apower & 0x8000) {
    apower= (apower & 0x7FFF) * -1;
  }
  return (double)apower / 1000;
}
double ATM90E32::GetApparentPowerC() {
  signed short apower = (signed short) CommEnergyIC(READ, SmeanC, 0xFFFF); 
  if (apower & 0x8000) {
    apower= (apower & 0x7FFF) * -1;
  }
  return (double)apower / 1000;
}
double ATM90E32::GetTotalApparentPower() {
  signed short apower = (signed short) CommEnergyIC(READ, SmeanT, 0xFFFF); 
  if (apower & 0x8000) {
    apower= (apower & 0x7FFF) * -1;
  }
  return (double)apower / 1000; //250
}

// FREQUENCY
double ATM90E32::GetFrequency() {
  unsigned short freq = CommEnergyIC(READ, Freq, 0xFFFF);
  return (double)freq / 100;
}

// POWER FACTOR
double ATM90E32::GetPowerFactorA() {
  short int pf = (short int) CommEnergyIC(READ, PFmeanA, 0xFFFF); 
  //if negative
  if (pf & 0x8000) {
    pf = (pf & 0x7FFF) * -1;
  }
  return (double)pf / 1000;
}
double ATM90E32::GetPowerFactorB() {
  short int pf = (short int) CommEnergyIC(READ, PFmeanB, 0xFFFF); 
  if (pf & 0x8000) {
    pf = (pf & 0x7FFF) * -1;
  }
  return (double)pf / 1000;
}
double ATM90E32::GetPowerFactorC() {
  short int pf = (short int) CommEnergyIC(READ, PFmeanC, 0xFFFF); 
  //if negative
  if (pf & 0x8000) {
    pf = (pf & 0x7FFF) * -1;
  }
  return (double)pf / 1000;
}
double ATM90E32::GetTotalPowerFactor() {
  short int pf = (short int) CommEnergyIC(READ, PFmeanT, 0xFFFF); 
  //if negative
  if (pf & 0x8000) {
    pf = (pf & 0x7FFF) * -1;
  }
  return (double)pf / 1000;
}

// PHASE ANGLE
double ATM90E32::GetPhaseA() {
  signed short apower = (signed short) CommEnergyIC(READ, PAngleA, 0xFFFF);
  return (double)apower / 10;
}
double ATM90E32::GetPhaseB() {
  signed short apower = (signed short) CommEnergyIC(READ, PAngleB, 0xFFFF);
  return (double)apower / 10;
}
double ATM90E32::GetPhaseC() {
  signed short apower = (signed short) CommEnergyIC(READ, PAngleC, 0xFFFF);
  return (double)apower / 10;
}

// TEMPERATURE
double ATM90E32::GetTemperature() {
  short int atemp = (short int) CommEnergyIC(READ, Temp, 0xFFFF); 
  return (double)atemp;
}

// ENERGY MEASUREMENT
double ATM90E32::GetImportEnergy() {
  unsigned short ienergyT = CommEnergyIC(READ, APenergyT, 0xFFFF);
  return (double)ienergyT / 100 / 3200; //returns kWh
}

double ATM90E32::GetExportEnergy() {
  unsigned short eenergyT = CommEnergyIC(READ, ANenergyT, 0xFFFF);
  return (double)eenergyT / 100 / 3200; //returns kWh 
}

/* System Status Registers */
// the status registers are different for the ATM90E32AS 
unsigned short ATM90E32::GetSysStatus0() {    
  return CommEnergyIC(READ, EMMIntState0, 0xFFFF);
}
unsigned short ATM90E32::GetSysStatus1() {
  return CommEnergyIC(READ, EMMIntState1, 0xFFFF);
}
unsigned short ATM90E32::GetMeterStatus0() {
  return CommEnergyIC(READ, EMMState0, 0xFFFF);
}
unsigned short ATM90E32::GetMeterStatus1() {
  return CommEnergyIC(READ, EMMState1, 0xFFFF);
}

/* Communication with ATM90E32 */
unsigned short ATM90E32::CommEnergyIC(unsigned char RW, unsigned short address, unsigned short val) 
{
  unsigned char* data = (unsigned char*)&val;
  unsigned char* adata = (unsigned char*)&address;
  unsigned short output;
  unsigned short address1;

  // SPISettings settings(200000, MSBFIRST, SPI_MODE2);
  // SPI.beginTransaction(settings);

  delayMicroseconds(10);

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
  delayMicroseconds(10);

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

  // SPI.endTransaction();

  output = (val >> 8) | (val << 8); //reverse MSB and LSB
  return output;
}

#if !defined(NO_GLOBAL_INSTANCES) 
ATM90E32 Monitoring;
#endif
