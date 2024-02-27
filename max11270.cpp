#include "max11270.h"

#define REGISTER_READ 1
#define REGISTER_WRITE 0
#define STATUS_REG 0
#define DATA_REG 0x6
#define SCYCLE 1
#define STARTBIT              0b10000000
#define MODE_REGISTER_ACCESS  0b01000000
#define MODE_CONVERSION       0b00000000
#define CALIBRATION_BIT       0b00100000
//#define SPI_CLOCK_SPEED 2400000
#define PGA_ENABLE  1<<3
#define DEFAULT_VREF 2.048

Max11270:: Max11270(int chip_select_pin)
     : cs(chip_select_pin), _vref(DEFAULT_VREF)
{

}

Max11270:: Max11270(int chip_select_pin, float vref)
     : cs(chip_select_pin), _vref(vref)
{
     _calc_raw2micro();
}


uint16_t Max11270 :: getStatusRegister()
{
     // Serial.print("writing ");
     // Serial.println(STARTBIT|MODE_REGISTER_ACCESS|STATUS_REG|REGISTER_READ, BIN);

     SPI.beginTransaction(SPISettings(spiClockSpeed, MSBFIRST, SPI_MODE0)); 
     // chip select
     digitalWrite(cs, LOW);
     // send read request with status register location
     SPI.transfer(STARTBIT|MODE_REGISTER_ACCESS|STATUS_REG|REGISTER_READ);
     // then read 2 bytes, send garbage while doing so
     //delayMicroseconds(5);
     uint8_t status1 = SPI.transfer(0xFF);
     uint8_t status2 = SPI.transfer(0xFF);
     digitalWrite(cs, HIGH); // end chip select
     SPI.endTransaction();
     //Serial.print(status1, HEX);Serial.print("  -  ");Serial.println(status2, HEX);
     return (((uint16_t)status1)<<8) | status2;

}
uint8_t Max11270 :: getCtrlRegister(uint8_t n)
{

     SPI.beginTransaction(SPISettings(spiClockSpeed, MSBFIRST, SPI_MODE0)); 
     // chip select
     digitalWrite(cs, LOW);
     // send read request with status register location
     SPI.transfer(STARTBIT|MODE_REGISTER_ACCESS|(n<<1)|REGISTER_READ);
     // then read 2 bytes, send garbage while doing so
     uint8_t value = SPI.transfer(0xFF);
     digitalWrite(cs, HIGH); // end chip select
     SPI.endTransaction();
     return value;
}

void Max11270 :: setCalibrationFactor(float cf)
{
     _calibration_factor = cf;
     _calc_raw2micro();
}
 
void Max11270 :: _calc_raw2micro()
{
     // the equation  is:

     //        Vref        raw_signed
     // V  = ---------- *  ------------ * calibration_factor
     //       max_range       gain

     // 8.388608 is the maximum raw value possible (0x800000) divided by 1e6 to
     // convert from Volts to MicroVolts.
     // TODO, calibration factor, vref, gain and the constant are all constants,
     // it shuold all be calculated beforehand, simplifying this calcluation at runtime
     // return  _calibration_factor*(_vref*raw_signed)/(_gain * 8.388608);
    
     _raw2micro = _calibration_factor*_vref/(_gain*8.388608);
}

void Max11270 :: setGain(uint8_t gain)
{
     // set the PGA enable bit (B3) and set the gain (B0-B2) of CTRL2
     _write_register(2, (ctrls[2] & 0xf0) | PGA_ENABLE | gain);
     _gain = (1<<gain);
     _calc_raw2micro();
}
void Max11270 :: setConversionMode(bool mode)
{
     // configure the SCYCLE bit (B1) of CTRL1
     _write_register(1, (ctrls[1] & 0b11111101) | (mode << 1));
}
void Max11270 :: setSyncMode(bool mode)
{
     // configure the SYNC bit (B6) of CTRL1
     _write_register(1, (ctrls[1] & 0b10111111) | (mode << 6));
}
void Max11270 :: setExternalClock(bool onoff)
{
     // configure the EXTCK bit (B7) of CTRL1
     _write_register(1, (ctrls[1] & 0b01111111) | (onoff << 7));

}
void Max11270 :: setDataRate(uint8_t datarate)
{
     // dataRate gets sent when conversion is started
     dataRate = datarate;
}
void _tbinprint(byte a)
{
  for (byte i = 0; i<8; i++)
  {
    Serial.print(0x01 & (a>>i));
    Serial.print("\t");
  }
}

void Max11270 :: _write_register(uint8_t register_n, uint8_t value)
{
     Serial.print("writing ");_tbinprint(value);Serial.print(" to CTRL ");Serial.println((int)register_n);

     // keep ctrls updated
     ctrls[register_n] = value;

     // writes one control register to max11270
     SPI.beginTransaction(SPISettings(spiClockSpeed, MSBFIRST, SPI_MODE0)); // start transmission
     digitalWrite(cs, LOW);// chip select is active low
     // PGA settings are in control register 2
     SPI.transfer(STARTBIT|MODE_REGISTER_ACCESS|(register_n << 1)|REGISTER_WRITE);
     // then send the value to be written
     SPI.transfer(ctrls[register_n]);
     // finally release chip select and end transaction
     digitalWrite(cs, HIGH);
     SPI.endTransaction();
}



void Max11270 :: performSelfCalibration()
{
     performSelfCalibration(true);
}
void Max11270 :: _requestCalibration(bool wait)
{
     ////// tells the max11270 to do a self calibration
     // Assumes that the calibration setting is properly set in CTRL5
     //Serial.println("... performing self calibration ... ");
     SPI.beginTransaction(SPISettings(spiClockSpeed, MSBFIRST, SPI_MODE0)); // start transmission
     digitalWrite(cs, LOW);// chip select
     SPI.transfer(STARTBIT|MODE_CONVERSION|CALIBRATION_BIT);
     // finally release chip select and end transaction
     digitalWrite(cs, HIGH);
     SPI.endTransaction();
     // self calibration takes 200 ms so sleep for a bit more
     if (wait)
     {
          delay(250);
     }
}

void Max11270 :: performSelfCalibration(bool wait)
{
     // first set the calibration bits (B7 and B6) of CTRL5 to 00
     _write_register(5, (ctrls[5] & 0b00111111));
     _requestCalibration(wait);
}


void Max11270 :: performZeroScaleCalibration()
{
     // first set the calibration bits (B7 and B6) of CTRL5 to 01
     _write_register(5, (ctrls[5] & 0b00111111) | 0b01000000);
     _requestCalibration(true);
}

// void Max11270  :: performFullScaleCalibration()
// {
//      // first set the calibration bits (B7 and B6) of CTRL5 to 10
//      _write_register(5, (ctrls[5] & 0b00111111) | 0b10000000);
//      _requestCalibration(true);
// }

void Max11270 :: startConversion()
{
     startConversion(dataRate);
}

void Max11270 :: startConversion(uint8_t datarate)
{
     SPI.beginTransaction(SPISettings(spiClockSpeed, MSBFIRST, SPI_MODE0)); // correct spi settings
     // chip select
     digitalWrite(cs, LOW);
     //  send conversion request along with the datarate
     SPI.transfer(STARTBIT|MODE_CONVERSION|datarate);
     // release chip select and end transaction
     digitalWrite(cs, HIGH);
     SPI.endTransaction();
}



uint32_t Max11270 :: read24bitRegister(uint8_t n)
{
     // Reads the nth registers, assumed to be 24 bit wide

     // maybe there should be some check on n here but idk

     // first send read request
     // begin SPI transaction, not strictly needed but good etiquette
     SPI.beginTransaction(SPISettings(spiClockSpeed, MSBFIRST, SPI_MODE0)); 
     // chip select
     digitalWrite(cs, LOW);
     // send read request 
     SPI.transfer(STARTBIT|MODE_REGISTER_ACCESS|(n<<1)|REGISTER_READ);
     // then read 3 bytes
     byte m1 = SPI.transfer(0xff);
     byte m2 = SPI.transfer(0xff);
     byte m3 = SPI.transfer(0xff);

     // finally release chip select to end transaction
     digitalWrite(cs, HIGH);
     SPI.endTransaction();

     // arrange the bytes into one uint32_t.
     return (m1<<16)|(m2<<8)|m3;
}


void Max11270 :: write24bitRegister(uint8_t n, uint32_t value)
{
     // writes to the nth registers, assumed to be 24 bit wide

     // maybe there should be some check on n here but idk

     // first send write request
     // begin SPI transaction, not strictly needed but good etiquette
     //SPI.beginTransaction(SPISettings(spiClockSpeed, MSBFIRST, SPI_MODE0)); 
     // chip select
     digitalWrite(cs, LOW);
     // send read request 
     SPI.transfer(STARTBIT|MODE_REGISTER_ACCESS|(n<<1)|REGISTER_WRITE);
     // write the three bytes
     SPI.transfer((uint8_t)((value>>16) &0xff));
     SPI.transfer((uint8_t)((value>>8) &0xff));
     SPI.transfer((uint8_t)((value) &0xff));

     // finally release chip select to end transaction
     digitalWrite(cs, HIGH);
     //SPI.endTransaction();

}

uint32_t Max11270 :: readRaw()
{
     return read24bitRegister(DATA_REG);
}


float Max11270 :: readMicroVolts()
{
     // read raw value
     uint32_t raw = readRaw();
     //Serial.print("raw=");Serial.println(raw, HEX);

     return convertRaw2MicroVolts(raw);
     
}

float Max11270 :: convertRaw2MicroVolts(unsigned long raw)
{
     // the max11270 maps the range +/- 0x800000 to +/- Vref.
     // First convert the raw recieved value to signed long by shifting the 24 bit value
     // to the left
     raw <<= 8;
     // then force cast without conversion 
     int32_t raw_signed = *(int32_t*)(void*)&raw;
     // shift it back to the right.
     raw_signed >>= 8; 

     return _raw2micro*raw_signed;
}



