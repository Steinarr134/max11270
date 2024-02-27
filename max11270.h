
#ifndef MAX11270
#define MAX11270

#include <Arduino.h>
#include <SPI.h>
#include <stdint.h>

// datarates, see table 9 of datasheet
#define MAX11270_CONTINUOUS_DATARATE_1_9        0       
#define MAX11270_CONTINUOUS_DATARATE_3_9        1
#define MAX11270_CONTINUOUS_DATARATE_7_8        2
#define MAX11270_CONTINUOUS_DATARATE_15_6       3
#define MAX11270_CONTINUOUS_DATARATE_31_2       4
#define MAX11270_CONTINUOUS_DATARATE_62_5       5
#define MAX11270_CONTINUOUS_DATARATE_125        6
#define MAX11270_CONTINUOUS_DATARATE_250        7
#define MAX11270_CONTINUOUS_DATARATE_500        8
#define MAX11270_CONTINUOUS_DATARATE_1K         9
#define MAX11270_CONTINUOUS_DATARATE_2K         10
#define MAX11270_CONTINUOUS_DATARATE_4K         11
#define MAX11270_CONTINUOUS_DATARATE_8K         12
#define MAX11270_CONTINUOUS_DATARATE_16K        13
#define MAX11270_CONTINUOUS_DATARATE_32K        14
#define MAX11270_CONTINUOUS_DATARATE_64K        15



// sync modes
#define MAX11270_CONTINUOUS_SYNC_MODE		1
#define MAX11270_PULSED_SYNC_MODE			0

// external clock mode
#define MAX11270_USE_EXTERNAL_CLOCK			1
#define MAX11270_USE_INTERNAL_CLOCK			0

// option to do calibration without waiting
#define MAX11270_CALIBRATION_NO_WAIT		0

// conversion modes
#define MAX11270_CONTINOUS_CONVERSION_MODE	0
#define MAX11270_SINGLE_CYCLE_MODE			1

// pga gains 
#define MAX11270_PGA_GAIN_1					0
#define MAX11270_PGA_GAIN_2					1
#define MAX11270_PGA_GAIN_4					2
#define MAX11270_PGA_GAIN_8					3
#define MAX11270_PGA_GAIN_16				4
#define MAX11270_PGA_GAIN_32				5
#define MAX11270_PGA_GAIN_64				6
#define MAX11270_PGA_GAIN_128				7


 class Max11270
 {
 	private :
 		uint32_t spiClockSpeed = 400000;
 		uint8_t dataRate = MAX11270_CONTINUOUS_DATARATE_1K;
 		float _vref;
 		float _calibration_factor = 1.;
 		int _gain = MAX11270_PGA_GAIN_1;
 		float _raw2micro;
 		uint8_t cs;
 		// ctrl resgisters are numbered 1,2,3,4 and 5, So to index them with their number
 		// the first value in the ctrls array is useless. These are the default values on
              // on startup so in theory the values in ctrls should be identical to the registers
              // inside the max11270
 		uint8_t ctrls[6] = {
 			0,
 			0b00000010,
 			0b00000000,
 			0b01100001,
 			0b00001111,
 			0b00001100
 		};
 		void _requestCalibration();
 		void _calc_raw2micro();

 	public :
 		Max11270(int chip_select_pin);
 		Max11270(int chip_select_pin, float vref);

 		// user is not expected to use this function but it is still public
 		// in case user wants to use settings not covered by this library.
 		void _write_register(uint8_t register_n, uint8_t value);

 		uint16_t getStatusRegister();
 		uint8_t getCtrlRegister(uint8_t n);

 		void setGain(uint8_t gain);
 		void setConversionMode(bool mode);
 		void setSyncMode(bool mode);
 		void setExternalClock(bool onoff);
 		void setDataRate(uint8_t datarate);
 		void setCalibrationFactor(float cf);
              void setSpiClockSpeed(uint32_t spi_clock);

 		void performSelfCalibration();
 		void performSelfCalibration(bool wait);
 		void performZeroScaleCalibration();
 		void performFullScaleCalibration(float microVolts);
 		void startConversion();	
 		void startConversion(uint8_t datarate);

 		uint32_t read24bitRegister(uint8_t n);
 		void write24bitRegister(uint8_t n, uint32_t value);


 		uint32_t readRaw();
        float convertRaw2MicroVolts(unsigned long raw);
 		float readMicroVolts();


 };
#endif
