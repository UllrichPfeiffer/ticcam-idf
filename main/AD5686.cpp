/*
Name:		AD5686.cpp
Created:	5/2/2018 9:45:56 AM
Author:	adi
Editor:	http://www.visualmicro.com
*/

#include "AD5686.h"

//SPIClass SPI(FSPI);
//SPIClass SPI(FSPI); // is VSPI

AD5686::AD5686() {}

AD5686::AD5686(SPISettings spiConf, uint8_t CSpin) {
    _spiConf = spiConf;
    _CSpin = CSpin;
    pinMode(_CSpin, OUTPUT);
    SPI.begin();
}

AD5686::AD5686(SPISettings spiConf, uint8_t CSpin, uint8_t ResetPin)
    :AD5686(spiConf, CSpin) {
    usesHardwareReset = true;
    _ResetPin = ResetPin;
    pinMode(_ResetPin, OUTPUT);
}

void AD5686::SetChannel(uint8_t channel, uint16_t vOut) {
    SPI.beginTransaction(_spiConf);
    digitalWrite(_CSpin, LOW);
    switch (channel) {
    case 0:
        SPI.transfer((CMD_WRITE_TO_INPUT_REG << 4) | ADDR_DAC_A);
        break;
    case 1:
        SPI.transfer((CMD_WRITE_TO_INPUT_REG << 4) | ADDR_DAC_B);
        break;
    case 2:
        SPI.transfer((CMD_WRITE_TO_INPUT_REG << 4) | ADDR_DAC_C);
        break;
    case 3:
        SPI.transfer((CMD_WRITE_TO_INPUT_REG << 4) | ADDR_DAC_D);
        break;
    default:
        return;
    }
    SPI.transfer(vOut >> 8);
    SPI.transfer(vOut);
    digitalWrite(_CSpin, HIGH);
    SPI.endTransaction();
}


void AD5686::SetReference(uint8_t source) {
    SPI.beginTransaction(_spiConf);
    digitalWrite(_CSpin, LOW);
    SPI.transfer(CMD_REFERENCE_SOURCE << 4);
    SPI.transfer(0);
    SPI.transfer(source);
    digitalWrite(_CSpin, HIGH);
    SPI.endTransaction();
}

void AD5686::SoftReset() {
    SPI.beginTransaction(_spiConf);
    digitalWrite(_CSpin, LOW);
    SPI.transfer(CMD_SOFWARE_RESET << 4);
    SPI.transfer(0);
    SPI.transfer(0);
    digitalWrite(_CSpin, HIGH);
    SPI.endTransaction();
}

void AD5686::HardReset() {
    if (usesHardwareReset) {
        digitalWrite(_ResetPin, LOW);
        delayMicroseconds(1);   //minimum reset pulse width time is 30ns
        digitalWrite(_ResetPin, HIGH);
    }
}

void AD5686::PowerDown(uint8_t operatingModeA, uint8_t operatingModeB) {
    SPI.beginTransaction(_spiConf);
    digitalWrite(_CSpin, LOW);
    SPI.transfer(CMD_POWER_UP_DOWN_DAC << 4);
    SPI.transfer(0);
    SPI.transfer(operatingModeB << 6 | 0xF << 2 | operatingModeA);
    digitalWrite(_CSpin, HIGH);
    SPI.endTransaction();
}
