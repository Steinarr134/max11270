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
#define PGA_ENABLE  1<<3

#define DEFAULT_VREF          2.048
#define DEFAULT_SPI_CLOCK     4000000

// this is an abstraction layer, all Arduino specific functions are
// in a #define so if someone wants to use the code on another platform
// then they only have to change this abstraction layer.

// macro for starting an spi transaction and activating chip select pin
#define SPI_BEGIN_TRANSACTION(CS, SPICLOCK) \
     do { \
          SPI.beginTransaction(SPISettings(SPICLOCK, MSBFIRST, SPI_MODE0)); \
          digitalWrite(CS, LOW);\
     } while (0)

// macro for transferring via spi
#define SPI_TRANSFER(b) (SPI.transfer(b))

// macro for ending transaction and deactivating chip select pin
#define SPI_END_TRANCSACTION(CS) \
     do { \
          digitalWrite(CS, HIGH); \
          SPI.endTransaction();  \
     } while (0)

// macro for delaying some amount of milliseconds
#define DELAY(MS) (delay(MS))


/* Here begins the actual coding of the class */

// Initializer, accept CS pin
Max11270:: Max11270(int chip_select_pin)
     : cs(chip_select_pin), _vref(DEFAULT_VREF)
{

}

// initializer, accept CS pin and the voltage on vref, default is 2.048 but maybe someone will use another value
Max11270:: Max11270(int chip_select_pin, float vref)
     : cs(chip_select_pin), _vref(vref)
{
     _calc_raw2micro();
}

// function to change the SPI clock frequency
void Max11270 :: setSpiClockSpeed(uint32_t spi_clock)
{
     spiClockSpeed = spi_clock;
}

// function that requests the status registers, reads it and returns as a 16 bit unsigned integer
uint16_t Max11270 :: getStatusRegister()
{
     // Serial.print("writing ");
     // Serial.println(STARTBIT|MODE_REGISTER_ACCESS|STATUS_REG|REGISTER_READ, BIN);

     // begin transmission
     SPI_BEGIN_TRANSACTION(spiClockSpeed, cs);
     // send read request with status register location
     SPI_TRANSFER(STARTBIT|MODE_REGISTER_ACCESS|STATUS_REG|REGISTER_READ);

     // then read 2 bytes, send garbage while doing so
     uint8_t status1 = SPI_TRANSFER(0xFF);
     uint8_t status2 = SPI_TRANSFER(0xFF);

     // end transaction
     SPI_END_TRANCSACTION(CS);

     //Serial.print(status1, HEX);Serial.print("  -  ");Serial.println(status2, HEX);
     return (((uint16_t)status1)<<8) | status2;
}

// // function that requests the status registers, reads it and returns as a 16 bit unsigned integer
// uint16_t Max11270 :: getStatusRegister()
// {
//      // Serial.print("writing ");
//      // Serial.println(STARTBIT|MODE_REGISTER_ACCESS|STATUS_REG|REGISTER_READ, BIN);

//      SPI.beginTransaction(SPISettings(spiClockSpeed, MSBFIRST, SPI_MODE0)); 
//      // chip select
//      digitalWrite(cs, LOW);
//      // send read request with status register location
//      SPI.transfer(STARTBIT|MODE_REGISTER_ACCESS|STATUS_REG|REGISTER_READ);
//      // then read 2 bytes, send garbage while doing so
//      //delayMicroseconds(5);
//      uint8_t status1 = SPI.transfer(0xFF);
//      uint8_t status2 = SPI.transfer(0xFF);
//      digitalWrite(cs, HIGH); // end chip select
//      SPI.endTransaction();
//      //Serial.print(status1, HEX);Serial.print("  -  ");Serial.println(status2, HEX);
//      return (((uint16_t)status1)<<8) | status2;

// }

// function that gets the control register
uint8_t Max11270 :: getCtrlRegister(uint8_t n)
{

     SPI.beginTransaction(SPISettings(spiClockSpeed, MSBFIRST, SPI_MODE0)); 
     // chip select
     digitalWrite(cs, LOW);
     // send read request with status register location
     SPI_TRANSFER(STARTBIT|MODE_REGISTER_ACCESS|(n<<1)|REGISTER_READ);
     // then read 2 bytes, send garbage while doing so
     uint8_t value = SPI_TRANSFER(0xFF);
     digitalWrite(cs, HIGH); // end chip select
     SPI.endTransaction();
     return value;
}

// to set the calibration factor.
void Max11270 :: setCalibrationFactor(float cf)
{
     _calibration_factor = cf;
     _calc_raw2micro();
}
 
 // function to calculate the constant that translates the raw value to microvolts
// by saving this one floating point constant the translation only costs one floating point
// multiplication.
void Max11270 :: _calc_raw2micro()
{
     // the equation  is:

     //        Vref        raw_signed
     // V  = ---------- *  ------------ * calibration_factor
     //       max_range       gain

     // 8.388608 is the maximum raw value possible (0x800000) divided by 1e6 to
     // convert from Volts to MicroVolts.
    
     _raw2micro = _calibration_factor*_vref/(_gain*8.388608);
}

// function to set the gain
void Max11270 :: setGain(uint8_t gain)
{
     // set the PGA enable bit (B3) and set the gain (B0-B2) of CTRL2
     _write_register(2, (ctrls[2] & 0xf0) | PGA_ENABLE | gain);

     // also configure gain in this library to calculate the correct microvolt level
     _gain = (1<<gain);
     _calc_raw2micro(); // calculate _raw2micro
}

// this sets the conversion mode,
// the options are continuous conversion and single cycle
void Max11270 :: setConversionMode(bool mode)
{
     // configure the SCYCLE bit (B1) of CTRL1, don't change other bits
     _write_register(1, (ctrls[1] & 0b11111101) | (mode << 1));
}

// this sets the sync mode on or off.
void Max11270 :: setSyncMode(bool mode)
{
     // configure the SYNC bit (B6) of CTRL1
     _write_register(1, (ctrls[1] & 0b10111111) | (mode << 6));
}

// this configures the max11270 to use external or internal clock signal
void Max11270 :: setExternalClock(bool onoff)
{
     // configure the EXTCK bit (B7) of CTRL1
     _write_register(1, (ctrls[1] & 0b01111111) | (onoff << 7));

}

// this sets the datarate
void Max11270 :: setDataRate(uint8_t datarate)
{
     // dataRate gets sent when conversion is started
     dataRate = datarate;
}

// this is a function to assist with the bits of registers
void _tbinprint(byte a)
{
  for (byte i = 0; i<8; i++)
  {
    Serial.print(0x01 & (a>>i));
    Serial.print("\t");
  }
}

// this is a function to assist in writing regitsers to the max11270
// register values are also stored in the ctrls array so that is is possible to change
// one bit of a register at a time without knowing the rest of the bits
void Max11270 :: _write_register(uint8_t register_n, uint8_t value)
{
     Serial.print("writing ");_tbinprint(value);Serial.print(" to CTRL ");Serial.println((int)register_n);

     // keep ctrls updated
     ctrls[register_n] = value;

     // writes one control register to max11270
     SPI.beginTransaction(SPISettings(spiClockSpeed, MSBFIRST, SPI_MODE0)); // start transmission
     digitalWrite(cs, LOW);// chip select is active low
     // PGA settings are in control register 2
     SPI_TRANSFER(STARTBIT|MODE_REGISTER_ACCESS|(register_n << 1)|REGISTER_WRITE);
     // then send the value to be written
     SPI_TRANSFER(ctrls[register_n]);
     // finally release chip select and end transaction
     digitalWrite(cs, HIGH);
     SPI.endTransaction();
}

// a function that requests the max11270 to perform a calibration,
// the type of calibration is controlled by the B7 and B6 bits of CTRL5
void Max11270 :: _requestCalibration()
{
     ////// tells the max11270 to do a self calibration
     // Assumes that the calibration setting is properly set in CTRL5
     //Serial.println("... performing self calibration ... ");
     SPI.beginTransaction(SPISettings(spiClockSpeed, MSBFIRST, SPI_MODE0)); // start transmission
     digitalWrite(cs, LOW);// chip select
     SPI_TRANSFER(STARTBIT|MODE_CONVERSION|CALIBRATION_BIT);
     // finally release chip select and end transaction
     digitalWrite(cs, HIGH);
     SPI.endTransaction();
}

// a function that instructs the max11270 to do an internal self calibration
void Max11270 :: performSelfCalibration(bool wait)
{
     // first set the calibration bits (B7 and B6) of CTRL5 to 00
     _write_register(5, (ctrls[5] & 0b00111111));
     _requestCalibration();
     // self calibration takes 200 ms so sleep for a bit more
     if (wait)
     {
          delay(250);
     }
}

// default  is wait=true
void Max11270 :: performSelfCalibration()
{
     performSelfCalibration(true);
}


// this is for zero scale calibration, it requires the user to short the inputs
// this function has not been tested and does not include a wait
void Max11270 :: performZeroScaleCalibration()
{
     // first set the calibration bits (B7 and B6) of CTRL5 to 01
     _write_register(5, (ctrls[5] & 0b00111111) | 0b01000000);
     _requestCalibration();
}

// void Max11270  :: performFullScaleCalibration()
// {
//      // first set the calibration bits (B7 and B6) of CTRL5 to 10
//      _write_register(5, (ctrls[5] & 0b00111111) | 0b10000000);
//      _requestCalibration(true);
// }

// instructs the max11270 to start a conversion
void Max11270 :: startConversion()
{
     SPI.beginTransaction(SPISettings(spiClockSpeed, MSBFIRST, SPI_MODE0)); // correct spi settings
     // chip select
     digitalWrite(cs, LOW);
     //  send conversion request along with the datarate
     SPI_TRANSFER(STARTBIT|MODE_CONVERSION|dataRate);
     // release chip select and end transaction
     digitalWrite(cs, HIGH);
     SPI.endTransaction();
}

// // default is to use the datarate configured
// void Max11270 :: startConversion()
// {
//      startConversion(dataRate);
// }


// helper function that reads a 24 bit register
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
     SPI_TRANSFER(STARTBIT|MODE_REGISTER_ACCESS|(n<<1)|REGISTER_READ);
     // then read 3 bytes
     byte m1 = SPI_TRANSFER(0xff);
     byte m2 = SPI_TRANSFER(0xff);
     byte m3 = SPI_TRANSFER(0xff);

     // finally release chip select to end transaction
     digitalWrite(cs, HIGH);
     SPI.endTransaction();

     // arrange the bytes into one uint32_t.
     return (m1<<16)|(m2<<8)|m3;
}

// helper function that writes a 24 bit register
void Max11270 :: write24bitRegister(uint8_t n, uint32_t value)
{
     // writes to the nth registers, assumed to be 24 bit wide

     // maybe there should be some check on n here but idk

     // first send write request
     // begin SPI transaction, not strictly needed but good etiquette
     SPI.beginTransaction(SPISettings(spiClockSpeed, MSBFIRST, SPI_MODE0)); 
     // chip select
     digitalWrite(cs, LOW);
     // send read request 
     SPI_TRANSFER(STARTBIT|MODE_REGISTER_ACCESS|(n<<1)|REGISTER_WRITE);
     // write the three bytes
     SPI_TRANSFER((uint8_t)((value>>16) &0xff));
     SPI_TRANSFER((uint8_t)((value>>8) &0xff));
     SPI_TRANSFER((uint8_t)((value) &0xff));

     // finally release chip select to end transaction
     digitalWrite(cs, HIGH);
     SPI.endTransaction();

}

// function that reads the data register
uint32_t Max11270 :: readRaw()
{
     return read24bitRegister(DATA_REG);
}

// the function to call when user wants a microvolt reading
float Max11270 :: readMicroVolts()
{
     // read raw value
     uint32_t raw = readRaw();
     //Serial.print("raw=");Serial.println(raw, HEX);

     return convertRaw2MicroVolts(raw);
     
}


// function to convert raw reading to microvolts
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



