/* ATM90E32 Energy Monitor Functions

  The MIT License (MIT)

  Copyright (c) 2016 whatnick,Ryzee and Arun

  Modified to use with the CircuitSetup.us Split Phase Energy Meter by jdeglavina

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "ATM90E32.h"
#include "Logger.h"
#include "JsonConfiguration.h"

ATM90E32::ATM90E32(void)
{
}

ATM90E32::~ATM90E32()
{
  // end
}

/* CommEnergyIC - Communication Establishment */
/*
  - Defines Register Mask
  - Treats the Register and SPI Comms
  - Outputs the required value in the register
*/
uint16_t ATM90E32::CommEnergyIC(uint8_t RW, uint16_t address, uint16_t val)
{
  uint16_t dataRead;
  const uint16_t addressCheck = LastSPIData | (1 << 15);
  const unsigned long timeout = millis() + 200;

  // Set R/W flag
  address |= RW << 15;

  if (RW == READ)
  {
    do
    {
      dataRead = readWriteSPI(address);
      delay(1);
    } while ((dataRead != readWriteSPI(addressCheck)) && (millis() < timeout));

    if (millis() >= timeout)
    {
      Log.println("ATM90E32::CommEnergyIC() => Time out during read at address 0x" + String(address, HEX));
      return 0;
    }

    return dataRead;
  }
  else if (RW == WRITE)
  {
    // Add exception for SoftReset (unable to read from addressCheck)
    if (address == SoftReset)
    {
      readWriteSPI(address, val);
      return val;
    }

    do
    {
      readWriteSPI(address, val);
      delay(1);
    } while ((val != readWriteSPI(addressCheck)) && (millis() < timeout));

    if (millis() >= timeout)
    {
      Log.println("ATM90E32::CommEnergyIC() => Time out during write at address 0x" + String(address, HEX));
      return 0;
    }

    return val;
  }

  return 0;
}

uint16_t ATM90E32::readWriteSPI(uint16_t address, uint16_t val)
{
  uint16_t output;

  SPISettings settings(200000, MSBFIRST, SPI_MODE3);

  // Transmit & Receive Data
  SPI.beginTransaction(settings);

  // Chip enable and wait for SPI activation
  digitalWrite(_cs, LOW);
  delayMicroseconds(10);

  SPI.write16(address);

  /* Must wait 4 us for data to become valid */
  delayMicroseconds(4);

  output = SPI.transfer16(val);

  // Chip enable and wait for transaction to end
  delayMicroseconds(10);
  digitalWrite(_cs, HIGH);

  SPI.endTransaction();

  return output;
}

int ATM90E32::Read32Register(signed short regh_addr, signed short regl_addr)
{
  int val, val_h, val_l;
  val_h = CommEnergyIC(READ, regh_addr);
  val_l = CommEnergyIC(READ, regl_addr);
  val = CommEnergyIC(READ, regh_addr);

  val = val_h << 16;
  val |= val_l; //concatenate the 2 registers to make 1 32 bit number

  return (val);
}

double ATM90E32::CalculateVIOffset(uint16_t regh_addr, uint16_t regl_addr /*, uint16_t offset_reg*/)
{
  //for getting the lower registers of Voltage and Current and calculating the offset
  //should only be run when CT sensors are connected to the meter,
  //but not connected around wires
  uint32_t val, val_h, val_l;
  uint16_t offset;
  val_h = CommEnergyIC(READ, regh_addr);
  val_l = CommEnergyIC(READ, regl_addr);
  val = CommEnergyIC(READ, regh_addr);

  val = val_h << 16; //move high register up 16 bits
  val |= val_l;      //concatenate the 2 registers to make 1 32 bit number
  val = val >> 7;    //right shift 7 bits - lowest 7 get ignored - V & I registers need this
  val = (~val) + 1;  //2s compliment

  offset = val; //keep lower 16 bits
  //CommEnergyIC(WRITE, offset_reg, (signed short)val);
  return uint16_t(offset);
}

double ATM90E32::CalculatePowerOffset(uint16_t regh_addr, uint16_t regl_addr /*, uint16_t offset_reg*/)
{
  //for getting the lower registers of energy and calculating the offset
  //should only be run when CT sensors are connected to the meter,
  //but not connected around wires
  uint32_t val, val_h, val_l;
  uint16_t offset;
  val_h = CommEnergyIC(READ, regh_addr);
  val_l = CommEnergyIC(READ, regl_addr);
  val = CommEnergyIC(READ, regh_addr);

  val = val_h << 16; //move high register up 16 bits
  val |= val_l;      //concatenate the 2 registers to make 1 32 bit number
  val = (~val) + 1;  //2s compliment

  offset = val; //keep lower 16 bits
  //CommEnergyIC(WRITE, offset_reg, (signed short)val);
  return uint16_t(offset);
}

double ATM90E32::CalibrateVI(uint16_t reg, uint16_t actualVal)
{
  //input the Voltage or Current register, and the actual value that it should be
  //actualVal can be from a calibration meter or known value from a power supply
  uint16_t gain, val, m, gainReg;
  //sample the reading
  val = CommEnergyIC(READ, reg);
  val += CommEnergyIC(READ, reg);
  val += CommEnergyIC(READ, reg);
  val += CommEnergyIC(READ, reg);

  //get value currently in gain register
  switch (reg)
  {
  case UrmsA:
  {
    gainReg = UgainA;
  }
  case UrmsB:
  {
    gainReg = UgainB;
  }
  case UrmsC:
  {
    gainReg = UgainC;
  }
  case IrmsA:
  {
    gainReg = IgainA;
  }
  case IrmsB:
  {
    gainReg = IgainB;
  }
  case IrmsC:
  {
    gainReg = IgainC;
  }
  }

  gain = CommEnergyIC(READ, gainReg);
  m = actualVal;
  m = ((m * gain) / val);
  gain = m;

  //write new value to gain register
  // CommEnergyIC(WRITE, gainReg, gain);

  return (gain);
}

/* Parameters Functions*/
/*
  - Gets main electrical parameters,
  such as: Voltage, Current, Power, Energy,
  and Frequency, and Temperature

*/
// VOLTAGE
double ATM90E32::GetLineVoltageA()
{
  uint16_t voltage = CommEnergyIC(READ, UrmsA);
  return (double)voltage / 100;
}
double ATM90E32::GetLineVoltageB()
{
  uint16_t voltage = CommEnergyIC(READ, UrmsB);
  return (double)voltage / 100;
}
double ATM90E32::GetLineVoltageC()
{
  uint16_t voltage = CommEnergyIC(READ, UrmsC);
  return (double)voltage / 100;
}

// CURRENT
double ATM90E32::GetLineCurrentA()
{
  uint16_t current = CommEnergyIC(READ, IrmsA);
  return (double)current / 1000;
}
double ATM90E32::GetLineCurrentB()
{
  uint16_t current = CommEnergyIC(READ, IrmsB);
  return (double)current / 1000;
}
double ATM90E32::GetLineCurrentC()
{
  uint16_t current = CommEnergyIC(READ, IrmsC);
  return (double)current / 1000;
}

double ATM90E32::GetLineCurrentN()
{
  uint16_t current = CommEnergyIC(READ, IrmsN);
  return (double)current / 1000;
}

// ACTIVE POWER
double ATM90E32::GetActivePowerA()
{
  int val = Read32Register(PmeanA, PmeanALSB);
  return (double)val * 0.00032;
}
double ATM90E32::GetActivePowerB()
{
  int val = Read32Register(PmeanB, PmeanBLSB);
  return (double)val * 0.00032;
}
double ATM90E32::GetActivePowerC()
{
  int val = Read32Register(PmeanC, PmeanCLSB);
  return (double)val * 0.00032;
}
double ATM90E32::GetTotalActivePower()
{
  int val = Read32Register(PmeanT, PmeanTLSB);
  return (double)val * 0.00032;
}

// Active Fundamental Power
double ATM90E32::GetTotalActiveFundPower()
{
  int val = Read32Register(PmeanTF, PmeanTFLSB);
  return (double)val * 0.00032;
}

// Active Harmonic Power
double ATM90E32::GetTotalActiveHarPower()
{
  int val = Read32Register(PmeanTH, PmeanTHLSB);
  return (double)val * 0.00032;
}

// REACTIVE POWER
double ATM90E32::GetReactivePowerA()
{
  int val = Read32Register(QmeanA, QmeanALSB);
  return (double)val * 0.00032;
}
double ATM90E32::GetReactivePowerB()
{
  int val = Read32Register(QmeanB, QmeanBLSB);
  return (double)val * 0.00032;
}
double ATM90E32::GetReactivePowerC()
{
  int val = Read32Register(QmeanC, QmeanCLSB);
  return (double)val * 0.00032;
}
double ATM90E32::GetTotalReactivePower()
{
  int val = Read32Register(QmeanT, QmeanTLSB);
  return (double)val * 0.00032;
}

// APPARENT POWER
double ATM90E32::GetApparentPowerA()
{
  int val = Read32Register(SmeanA, SmeanALSB);
  return (double)val * 0.00032;
}
double ATM90E32::GetApparentPowerB()
{
  int val = Read32Register(SmeanB, SmeanBLSB);
  return (double)val * 0.00032;
}
double ATM90E32::GetApparentPowerC()
{
  int val = Read32Register(SmeanC, SmeanCLSB);
  return (double)val * 0.00032;
}
double ATM90E32::GetTotalApparentPower()
{
  int val = Read32Register(SmeanT, SAmeanTLSB);
  return (double)val * 0.00032;
}

// FREQUENCY
double ATM90E32::GetFrequency()
{
  uint16_t freq = CommEnergyIC(READ, Freq);
  return (double)freq / 100;
}

// POWER FACTOR
double ATM90E32::GetPowerFactorA()
{
  signed short pf = (signed short)CommEnergyIC(READ, PFmeanA);
  return (double)pf / 1000;
}
double ATM90E32::GetPowerFactorB()
{
  signed short pf = (signed short)CommEnergyIC(READ, PFmeanB);
  return (double)pf / 1000;
}
double ATM90E32::GetPowerFactorC()
{
  signed short pf = (signed short)CommEnergyIC(READ, PFmeanC);
  return (double)pf / 1000;
}
double ATM90E32::GetTotalPowerFactor()
{
  signed short pf = (signed short)CommEnergyIC(READ, PFmeanT);
  return (double)pf / 1000;
}

// MEAN PHASE ANGLE
double ATM90E32::GetPhaseA()
{
  uint16_t angleA = (uint16_t)CommEnergyIC(READ, PAngleA);
  return (double)angleA / 10;
}
double ATM90E32::GetPhaseB()
{
  uint16_t angleB = (uint16_t)CommEnergyIC(READ, PAngleB);
  return (double)angleB / 10;
}
double ATM90E32::GetPhaseC()
{
  uint16_t angleC = (uint16_t)CommEnergyIC(READ, PAngleC);
  return (double)angleC / 10;
}

// TEMPERATURE
double ATM90E32::GetTemperature()
{
  short int atemp = (short int)CommEnergyIC(READ, Temp);
  return (double)atemp;
}

/* Gets the Register Value if Desired */
// REGISTER
double ATM90E32::GetValueRegister(uint16_t registerRead)
{
  return (double)CommEnergyIC(READ, registerRead); //returns value register
}

// REGULAR ENERGY MEASUREMENT

// FORWARD ACTIVE ENERGY
// these registers accumulate energy and are cleared after being read
double ATM90E32::GetImportEnergy()
{
  uint16_t ienergyT = CommEnergyIC(READ, APenergyT);
  return (double)ienergyT / 100 / 3200; //returns kWh
}

// FORWARD REACTIVE ENERGY
double ATM90E32::GetImportReactiveEnergy()
{
  uint16_t renergyT = CommEnergyIC(READ, RPenergyT);
  return (double)renergyT / 100 / 3200; //returns kWh
}

// APPARENT ENERGY
double ATM90E32::GetImportApparentEnergy()
{
  uint16_t senergyT = CommEnergyIC(READ, SAenergyT);
  return (double)senergyT / 100 / 3200; //returns kWh
}

// REVERSE ACTIVE ENERGY
double ATM90E32::GetExportEnergy()
{
  uint16_t eenergyT = CommEnergyIC(READ, ANenergyT);
  return (double)eenergyT / 100 / 3200; //returns kWh
}

// REVERSE REACTIVE ENERGY
double ATM90E32::GetExportReactiveEnergy()
{
  uint16_t reenergyT = CommEnergyIC(READ, RNenergyT);
  return (double)reenergyT / 100 / 3200; //returns kWh
}

/* System Status Registers */
uint16_t ATM90E32::GetSysStatus0()
{
  return CommEnergyIC(READ, EMMIntState0);
}
uint16_t ATM90E32::GetSysStatus1()
{
  return CommEnergyIC(READ, EMMIntState1);
}
uint16_t ATM90E32::GetMeterStatus0()
{
  return CommEnergyIC(READ, EMMState0);
}
uint16_t ATM90E32::GetMeterStatus1()
{
  return CommEnergyIC(READ, EMMState1);
}

/* BEGIN FUNCTION */
/*
  - Define the pin to be used as Chip Select
  - Set serialFlag to true for serial debugging
  - Use SPI MODE 0 for the ATM90E32
*/
void ATM90E32::begin(int pin, uint16_t lineFreq, uint16_t pgagain, uint16_t ugain, uint16_t igainA, uint16_t igainB, uint16_t igainC)
{
  _cs = pin;                                             // SS PIN
  _lineFreq = (lineFreq == MODE_MONO) ? 0x0087 : 0x0185; // frequency of power
  _pgagain = pgagain;                                    // PGA Gain for current channels
  _ugain = ugain;                                        // voltage rms gain

  digitalWrite(_cs, HIGH);
  pinMode(_cs, OUTPUT);

  /* Set MODE to Normal MODE */
  pinMode(ATM90E32_PM0, OUTPUT);
  digitalWrite(ATM90E32_PM0, LOW);
  pinMode(ATM90E32_PM1, OUTPUT);
  digitalWrite(ATM90E32_PM1, LOW);

  delay(100);

  digitalWrite(ATM90E32_PM0, HIGH);
  digitalWrite(ATM90E32_PM1, HIGH);

  delay(100);

  /* Enable SPI */
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

  Log.println("Connecting to ATM90E32");

  const uint16_t sagV = 190; // in Volts
  const uint16_t FreqHiThresh = 51 * 100;
  const uint16_t FreqLoThresh = 49 * 100;

  const uint16_t vSagTh = (double(sagV * 100) * sqrt(2)) / (double(2 * _ugain) / 32768.f);

  //Initialize registers
  CommEnergyIC(WRITE, SoftReset, 0x789A); // 70 Perform soft reset
  delay(100);
  CommEnergyIC(WRITE, CfgRegAccEn, 0x55AA); // 7F enable register config access
  delay(10);
  CommEnergyIC(WRITE, MeterEn, 0x0001); // 00 Enable Metering

  CommEnergyIC(WRITE, SagPeakDetCfg, 0x143F);  // 05 Sag and Voltage peak detect period set to 20ms
  CommEnergyIC(WRITE, OVth, 0xFFFF);           // 06 Over Voltage Threshold
  CommEnergyIC(WRITE, SagTh, vSagTh);          // 08 Voltage sag threshold
  CommEnergyIC(WRITE, FreqHiTh, FreqHiThresh); // 0D High frequency threshold
  CommEnergyIC(WRITE, FreqLoTh, FreqLoThresh); // 0C Lo frequency threshold
  CommEnergyIC(WRITE, EMMIntEn0, 0x0000);      // 75 Disable interrupts
  CommEnergyIC(WRITE, EMMIntEn1, 0x0000);      // 76 Disable interrupts
  CommEnergyIC(WRITE, EMMIntState0, 0xFFFF);   // 73 Clear interrupt flags
  CommEnergyIC(WRITE, EMMIntState1, 0xFFFF);   // 74 Clear interrupt flags
  CommEnergyIC(WRITE, ZXConfig, 0x0001);       // 07 ZX2, ZX1, ZX0 pin config - set to current channels, all polarity

  //Set metering config values (CONFIG)
  CommEnergyIC(WRITE, PLconstH, 0x0861);  // 31 PL Constant MSB (default) - Meter Constant = 3200 - PL Constant = 140625000
  CommEnergyIC(WRITE, PLconstL, 0xC468);  // 32 PL Constant LSB (default) - this is 4C68 in the application note, which is incorrect
  CommEnergyIC(WRITE, MMode0, _lineFreq); // 33 Mode Config (frequency set in main program)
  CommEnergyIC(WRITE, MMode1, _pgagain);  // 34 PGA Gain Configuration for Current Channels - 0x002A (x4) // 0x0015 (x2) // 0x0000 (1x)
  CommEnergyIC(WRITE, PStartTh, 0x1D4C);  // 35 All phase Active Startup Power Threshold - 50% of startup current = 0.02A/0.00032 = 7500
  CommEnergyIC(WRITE, QStartTh, 0x1D4C);  // 36 All phase Reactive Startup Power Threshold
  CommEnergyIC(WRITE, SStartTh, 0x1D4C);  // 37 All phase Apparent Startup Power Threshold
  CommEnergyIC(WRITE, PPhaseTh, 0x02EE);  // 38 Each phase Active Phase Threshold = 10% of startup current = 0.002A/0.00032 = 750
  CommEnergyIC(WRITE, QPhaseTh, 0x02EE);  // 39 Each phase Reactive Phase Threshold
  CommEnergyIC(WRITE, SPhaseTh, 0x02EE);  // 3A Each phase Apparent Phase Threshold

  // Set metering calibration values (CALIBRATION)
  CommEnergyIC(WRITE, PoffsetA, 0x0000); // 41 A line active power offset FFDC
  CommEnergyIC(WRITE, QoffsetA, 0x0000); // 42 A line reactive power offset
  CommEnergyIC(WRITE, PoffsetB, 0x0000); // 43 B line active power offset
  CommEnergyIC(WRITE, QoffsetB, 0x0000); // 44 B line reactive power offset
  CommEnergyIC(WRITE, PoffsetC, 0x0000); // 45 C line active power offset
  CommEnergyIC(WRITE, QoffsetC, 0x0000); // 46 C line reactive power offset
  CommEnergyIC(WRITE, PQGainA, 0x0000);  // 47 Line calibration gain
  CommEnergyIC(WRITE, PhiA, 0x0000);     // 48 Line calibration angle
  CommEnergyIC(WRITE, PQGainB, 0x0000);  // 49 Line calibration gain
  CommEnergyIC(WRITE, PhiB, 0x0000);     // 4A Line calibration angle
  CommEnergyIC(WRITE, PQGainC, 0x0000);  // 4B Line calibration gain
  CommEnergyIC(WRITE, PhiC, 0x0000);     // 4C Line calibration angle

  // Set metering calibration values (HARMONIC)
  CommEnergyIC(WRITE, POffsetAF, 0x0000); // 51 A Fund. active power offset
  CommEnergyIC(WRITE, POffsetBF, 0x0000); // 52 B Fund. active power offset
  CommEnergyIC(WRITE, POffsetCF, 0x0000); // 53 C Fund. active power offset
  CommEnergyIC(WRITE, PGainAF, 0x0000);   // 54 A Fund. active power gain
  CommEnergyIC(WRITE, PGainBF, 0x0000);   // 55 B Fund. active power gain
  CommEnergyIC(WRITE, PGainCF, 0x0000);   // 56 C Fund. active power gain

  // Set measurement calibration values (ADJUST)
  CommEnergyIC(WRITE, UgainA, _ugain);                    // 61 A Voltage rms gain
  CommEnergyIC(WRITE, IgainA, igainA * ATM90E32_IGAIN);   // 62 A line current gain
  CommEnergyIC(WRITE, UoffsetA, 0x0000);                  // 63 A Voltage offset (-7056)
  CommEnergyIC(WRITE, IoffsetA, 0x0000);                  // 64 A line current offset (-928)
  CommEnergyIC(WRITE, UgainB, _ugain);                    // 65 B Voltage rms gain
  CommEnergyIC(WRITE, IgainB, igainB * ATM90E32_IGAIN);   // 66 B line current gain
  CommEnergyIC(WRITE, UoffsetB, 0x0000);                  // 67 B Voltage offset (-3072)
  CommEnergyIC(WRITE, IoffsetB, 0x0000);                  // 68 A line current offset (-928)
  CommEnergyIC(WRITE, UgainC, _ugain);                    // 69 C Voltage rms gain
  CommEnergyIC(WRITE, IgainC, igainC * ATM90E32_IGAIN);   // 6A C line current gain
  CommEnergyIC(WRITE, UoffsetC, 0x0000);                  // 6B C Voltage offset (-928)
  CommEnergyIC(WRITE, IoffsetC, 0x0000);                  // 6C A line current offset (-928)

  delay(10);
  CommEnergyIC(WRITE, CfgRegAccEn, 0x0000); // 7F end configuration
}

/* Read all data from ATM90E32 device */
void ATM90E32::handle(void)
{
  static unsigned long oldTick;
  double time = (millis() - oldTick) / 1000;
  oldTick = millis();

  double temp = GetTemperature();
  Log.println("Temperature: " + String(temp) + "°C");
  _line_A.voltage = GetLineVoltageA();
  _line_B.voltage = GetLineVoltageB();
  _line_C.voltage = GetLineVoltageC();
  _line_A.cosPhy = GetPowerFactorA();
  _line_B.cosPhy = GetPowerFactorB();
  _line_C.cosPhy = GetPowerFactorC();

  if (Configuration._mode == MODE_MONO)
  {
    if (Configuration._enableA) {
      _line_A.current = GetLineCurrentA();
      _line_A.power = GetActivePowerA();
      _line_A.conso += (_line_A.power / 3600) * time / 1000;
    }
    if (Configuration._enableB) {
      _line_B.current = GetLineCurrentB();
      _line_B.power = GetActivePowerB();
      _line_B.conso += (_line_B.power / 3600) * time / 1000;
    }
    if (Configuration._enableC) {
      _line_C.power = GetActivePowerC();
      _line_C.current = GetLineCurrentC();
      _line_C.conso += (_line_C.power / 3600) * time / 1000;
    }
  }
  else if (Configuration._mode == MODE_TRI_1)
  {
    if (Configuration._enableA) {
      _line_A.current = (GetLineCurrentA() * (2 * sqrt(3)) / 3);
      _line_A.current += (GetLineCurrentC() * sqrt(3) / 3);
      _line_A.power = (GetActivePowerA() * (2 * sqrt(3)) / 3);
      _line_A.power += (GetActivePowerC() * sqrt(3) / 3);
      _line_A.conso += (_line_A.power / 3600) * time / 1000;
    }
    _line_B.current = 0;
    _line_B.power = 0;
    _line_B.conso = 0;
    _line_C.current = 0;
    _line_C.power = 0;
    _line_C.conso = 0;
  }
  else if (Configuration._mode == MODE_TRI_2)
  {
    if (Configuration._enableA) {
      _line_A.current = GetLineCurrentA() * sqrt(3);
      _line_A.power = GetActivePowerA() * sqrt(3);
      _line_A.conso += (_line_A.power / 3600) * time / 1000;
    }
    _line_B.current = 0;
    _line_B.power = 0;
    _line_B.conso = 0;
    if (Configuration._enableC) {
      _line_C.current = GetLineCurrentC() * sqrt(3);
      _line_C.power = GetActivePowerC() * sqrt(3);
      _line_C.conso += (_line_C.power / 3600) * time / 1000;
    }
  }
  else if (Configuration._mode == MODE_DEBUG)
  {
    _line_A.current = GetLineCurrentA();
    _line_B.current = GetLineCurrentB();
    _line_C.current = GetLineCurrentC();
    _line_A.power = GetActivePowerA();
    _line_B.power = GetActivePowerB();
    _line_C.power = GetActivePowerC();
    _line_A.conso += (_line_A.power / 3600) * time / 1000;
    _line_B.conso += (_line_B.power / 3600) * time / 1000;
    _line_C.conso += (_line_C.power / 3600) * time / 1000;
  }

  Log.println(String(Configuration._nameA) + ": " + String(_line_A.voltage) + "V, " + String(_line_A.current) + "A, " + String(_line_A.power) + "W, " + String(_line_A.conso) + "kW/h, cos phy " + String(_line_A.cosPhy));
  Log.println(String(Configuration._nameB) + ": " + String(_line_B.voltage) + "V, " + String(_line_B.current) + "A, " + String(_line_B.power) + "W, " + String(_line_B.conso) + "kW/h, cos phy " + String(_line_B.cosPhy));
  Log.println(String(Configuration._nameC) + ": " + String(_line_C.voltage) + "V, " + String(_line_C.current) + "A, " + String(_line_C.power) + "W, " + String(_line_C.conso) + "kW/h, cos phy " + String(_line_C.cosPhy));
  Log.println("Frequency: " + String(Monitoring.GetFrequency()) + "Hz");
}

#if !defined(NO_GLOBAL_INSTANCES)
ATM90E32 Monitoring;
#endif
