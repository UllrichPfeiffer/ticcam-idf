/*
Simple library for driving Analog Devices AD5689/R digital to analog converter.

 Name:		AD5686.h
 Created:	5/2/2018 4:16:22 PM
 Author:	Alek Krol adi@adigital.eu
 Editor:	http://www.visualmicro.com
 Modified:  02/28/22 U. Pfeiffer for quad dac from AD5689 to AD5686
*/

#ifndef _AD5686_h
#define _AD5686_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include <SPI.h>

#define CMD_WRITE_TO_INPUT_REG          0b0001	//dependent on LDAC input
#define CMD_UPDATE_DAC_WITH_INPUT_REG   0b0010
#define CMD_WRITE_AND_UPDATE_DAC        0b0011
#define CMD_POWER_UP_DOWN_DAC           0b0100
#define CMD_HARDWARE_LDAC_MASK          0b0101
#define CMD_SOFWARE_RESET               0b0110
#define CMD_REFERENCE_SOURCE            0b0111
#define CMD_DAISY_CHAIN_ENABLE          0b1000
#define CMD_READBACK_ENABLE             0b1001
#define ADDR_DAC_A                      0b0001
#define ADDR_DAC_B                      0b0010
#define ADDR_DAC_C                      0b0100
#define ADDR_DAC_D                      0b1000
#define ADDR_DAC_AB                     0b0011
#define ADDR_DAC_ABCD                   0b1111

// #define INTERNAL                            0
// #define EXTERNAL                            1

class AD5686 {
public:
    AD5686();
    AD5686(SPISettings spiConf, uint8_t CSpin);
    AD5686(SPISettings spiConf, uint8_t CSpin, uint8_t ResetPin);
    void SetChannel(uint8_t channel, uint16_t vOut);
    void SetReference(uint8_t source);
    void SoftReset();
    void HardReset();
    void PowerDown(uint8_t operatingModeA, uint8_t operatingModeB); //0-normal, 1-1kOhm to ground, 2-100kOHm to ground, 3-three-state
private:
    SPISettings _spiConf;
    uint8_t		_CSpin;
    uint8_t		_ResetPin;
    bool        usesHardwareReset = false;
};

#endif
