// TicWave GmbH, U. Pfeiffer 15.3.2023
// Main header

#ifndef MAIN_H
#define MAIN_H

//#include <M5Core2.h>
//#include <lvgl.h>
//#include "global_var.h"
#include "sensor.h"
#include "stddef.h"
#include "sys/time.h"
#include <cJSON.h>

#define MODEL CONFIG_MODEL //"TicCAM-3.3kpix"
#define FIRMWARE CONFIG_FIRMWARE //"TW_ID03"
#define REVISION CONFIG_REVISION //"1a" 
static const char* TAG = "twid03";
#define CONFIG_ARDUHAL_LOG_DEFAULT_LEVEL 0

#define SD_FAT_TYPE 3
#define SPI_SPEED SD_SCK_MHZ(10) // MHz: OK 4, 10, 20, 25  ->  too much: 29, 30, 40, 50 causes errors
#define SD_CONFIG SdSpiConfig(TFCARD_CS_PIN, SHARED_SPI, SPI_SPEED) // TFCARD_CS_PIN is defined in M5Stack Config.h (Pin 4)
// also in file SdFat\src\SdFatConfig.h    at Line 100:
// #define ENABLE_DEDICATED_SPI 0        // (from 1 -> 0)


//#define WITH_OTA 

// change Display SPI speed in components/M5Core2/src/utility/In_eSPI.h 
//#define SPI_FREQUENCY 20000000 // default speed for display
// change the PSRAM speed with idf.py menuconfir to 80MHz
#define ASIC_SPI_Clock  200000 // 20MHz

#define DEFAULT_SERIAL_BAUDRATE 2000000 // 2000000 observed data arrors at 2000000
//#define DEFAULT_SERIAL_BAUDRATE 115200
#define DEFAULT_SERIAL_BUFFER_LENGTH 256
//#define CANVAS_WIDTH  240
//#define CANVAS_HEIGHT 240
#define LCD_WIDTH 320
#define LCD_HEIGHT 240
//#define NUMXY 33

    #if CONFIG_TWID02_SUPPORT
#define NUMX 33
#define NUMY 33
#define PIXEL_WIDTH 7
#define PIXEL_HEIGHT 7
    #elif CONFIG_TWID03_SUPPORT
#define NUMX 64
#define NUMY 48
#define PIXEL_WIDTH 3
#define PIXEL_HEIGHT 3
    #elif CONFIG_TWID04_SUPPORT
#define NUMX 64
#define NUMY 4
#define PIXEL_WIDTH 3
#define PIXEL_HEIGHT 30
    #else
#error "No valid CONFIG_TWIDxx_SUPPORT defined!"
    #endif


#define BYTES_PER_PIXEL 2
#define HISTOGRAM_RANGE_X 13
#define HISTOGRAM_RANGE_Y 99
#define SCREEN_TAB_WIDTH 40

/* You only need to format SPIFFS the first time you run a
   test or else use the SPIFFS plugin to create a partition
   https://github.com/me-no-dev/arduino-esp32fs-plugin */
#define FORMAT_SPIFFS_IF_FAILED true

 //++++ Used for external chopping Signal +++++
#define PWM_CH_PELTIER 1
#define PWM_CH_PUMP 2
#define PWM_CH_FAN 3
#define PWM_CH_CHOP 4

#define LDO_NUM_FAN 3

// NTP
#define NTP_TIMEZONE  "CET-1CEST,M3.5.0,M10.5.0/3" // see https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv for Berlin
#define NTP_SERVER1   "0.de.pool.ntp.org"
#define NTP_SERVER2   "1.de.pool.ntp.org"
#define NTP_SERVER3   "2.de.pool.ntp.org"

// WIFI
//#define WIFI_SSID "raysecur"
//#define WIFI_PASSWD "tw_id02"
#define WIFI_SSID "ExtNet"
//#define WIFI_SSID_AP ""
#define WIFI_SSID_AP "TicWaveCam"
//#define WIFI_PASSWD  "TopticaPho"
#define WIFI_PASSWD "ticwave4cam"
//#define WIFI_PASSWD "TuDarmstadt"
//#define WIFI_SSID "ESPbridge"
//#define WIFI_PASSWD "tw_bridge"


#define WIFI_TCP_PORT 5000 // remote port
#define WIFI_OTA_PORT 3232 // remote port
#define MAX_SRV_CLIENTS 1 //how many clients should be able to telnet to this ESP32

#define FB_GET_TIMEOUT (4000 / portTICK_PERIOD_MS)
// Define passwd for private control functions
#define SHA256_LENGTH 32

//WiFiClient Client;

//const char * firmware = "tw_id01_v1"; 
// DAC index
#define IDX_DAC_DET_GATE 0
#define IDX_DAC_VREF 1
#define IDX_DAC_VBB_X2 2
#define IDX_DAC_NC 3

// POT index
#define IDX_POT_REF 0
#define IDX_POT_DET 1
#define IDX_POT_VCO 2

// PWM index
#define IDX_PWM_PELTIER 0
#define IDX_PWM_PUMP 1
#define IDX_PWM_FAN 2

// Digital Pot
#define POT_ADDR            0x2F // 0b0101111 // = 0x2F (LSBs have pullup on A0, A1, A2)
#define POT_ADDR1           0x2B // 0b0101011 // = 0x2F (LSBs have pullup on A0, A1, A2)
#define POT_ADDR2           0x2D // 0b0101101 // = 0x2F (LSBs have pullup on A0, A1, A2)
#define POT_ADDR3           0x29 // 0b0101001 // = 0x2F (LSBs have pullup on A0, A1, A2)
#define POT_ADDR4           0x2E // 0b0101110 // = 0x2F (LSBs have pullup on A0, A1, A2)
#define POT_RANGE   255

#define delayShort 10 // ms
#define delayLong 300 //ms

// POT Regs
#define RegVolatWiperCH0     0x00
#define RegVolatWiperCH1     0x01
#define RegNonVolatWiperCH0  0x02
#define RegNonVolatWiperCH1  0x03
#define TCON               0x04
#define RegPotStatus       0x05 

// common register adresses for sweeps 
#define SWEEP_VTRIM 13
#define SWEEP_PIX_DAC 19
#define SWEEP_VGATE 11
#define SWEEP_GAIN1 0
#define SWEEP_GAIN2 1
#define SWEEP_PIX 9
//#define M5_Core2_ID01_V1

#define ADC_BITS_ESP 12
#define ADC_BITS_AD7924 12
#define ADC_RANGE_AD7924 4096
//#define RANGE_TWID02 65535 //16-bit AD5686 dac
#define RANGE_TWID02 32768 // 15 bis is default, 65535 might be requires for averaging INT16_MAX for int?
#define ADC_RANGE_ADS1100 65536
#define ADC_RANGE_ESP 4096
#define FFT_SAMPLES 512 //This value MUST ALWAYS be a power of 2
#define FFT_TYPE_NORMAL 0
#define FFT_TYPE_GOERTZEL 1

#define SCOPE_TYPE_IDLE 0
#define SCOPE_TYPE_TIME 1
#define SCOPE_TYPE_FREQ 2
#define SCOPE_TYPE_SPEC 3
#define SCOPE_TYPE_VIDEO 4
#define SCOPE_TYPE_STREAM 5

// configure camera release features here
// now done in Kconfig.projbuild use idf.py menuconfig instead
//#define CONFIG_MENU_TAB2 1 // sliders tab 
//#define CONFIG_MENU_TAB3 1 // feature settings tab
//#define CONFIG_MENU_TAB4 0 // SPI chip config tab
//#define CONFIG_BIAS_SLIDERS 0
//#define CONFIG_BILINEAR_INTERPOLATION 0
//#define CONFIG_VIDEO_FILTER 0
//#define CONFIG_MONITOR_VERBOSE 1 // extra verbose level, e.g.chip ID
//#define CONFIG_WIFI_AP 1
//#define CONFIG_LDOFAN_ENABLE 0
//#define CONFIG_EXTENDED_CLK_SETINGS 0
//#define CONFIG_VIDEOCONTROLLER 0 // extra features
//#define CONFIG_IMAGE_ZOOM 0

#define DAC_RANGE_AD5696 65536 //16-bit AD5686 dac
#define DAC_RANGE_AD5696_REDUCED 4096 //reduce to 12-bit dac
#define DAC_BIT_SHIFT 4 //bistshift to reduce to 12-bit dac

//GPIO_NTCG164BF103HT1: GPIO_NTC(G multi layer) 16 (1608 this seems not corrct it is a 0603) 4B (Bconst=4051-4100) F103(nom res=10k) H(+-3%)T(taping)1()
#define R_NTC_pull 10000 //10k GPIO_NTC pullup resistor to 3.3V
#define R_NTC 10000 //Resistance of GPIO_NTC 
//#define B_NTC 4070 // 4070  // From datasheet in K
#define B_NTC 3380 // for NTCG103JF103FT1 1% 1k From datasheet in K
#define T0_NTC 298.15   // temp (25°C) wich belongs to R_NTC in K

#define TEMP_Tcal_degC 22.2 // calibration temp
#define TEMP_Vcal_mV 2266 // 2229 calibration temp
#define TEMP_coeff 3 // in mV/K (is a neg. coeff)
#define TEMP_OFF_MAX_degC 80.0 // Max T for Thermal Shutdown
#define TEMP_OFF_MIN_degC -10.0 // Max T for Thermal Shutdown
#define TEMP_ON_degC 22.0 // Max T for Thermal Shutdown

// NTCG063JF103FTB, NTC-Thermistoren Case EIA 0201 B= 1% ,10kOhm 1%

#define TIMER_PERIOD_MS 5000

#define PWM_RES_BITS 12 // ESP32 uses 8, 10, 12, 15-bit
#define PWM_RES 4095 // 2^PWM_RES_BITS-1
#define PWM_FREQ_PELTIER 1000
#define PWM_FREQ_PUMP 10000
#define PWM_FREQ_FAN 10000 // External pin 
#define PWM_FREQ_CHOP 70 // value used together with TK Measurement (35Hz)
//#define FW_ID01_V1

// the esp_camera part

#define FW_ID02_V2

#if defined(FW_ID01_V1) // use this for ticwave brewed M5 Core2 module
 #define GPIO_TEST_VCC 36 // not used for now
 #define GPIO_TEST_GND 34 //34
 #define GPIO_NTC 35 
 #define GPIO_PUMP_PWM 2
 #define GPIO_FAN_PWM 25
 #define GPIO_PELTIER_PWM 33// 37 // GPIOs 34 to 39 can't generate PWM!!!
 #define GPIO_FAN_RPM 32
 #define GPIO_PUMP_RPM 37 // 33 // change with GPIO_PELTIER_PWM for now
 #define GPIO_CLK_CHOP_IN 12
 #define GPIO_ADC_SS 0 // 17 is in clonflict with PSRAM sclk -> can not use PSRAM on source!!!
//also #define CONFIG_D0WD_PSRAM_CS_IO 4 in sdkconfig through idf.py menuconfig
 #define TFCARD_CS_PIN 16 // define this in ./components/M5Core2/src/utility/Config.h
// CONFIG_D0WD_PSRAM_CS_IO=4 with idf.py menuconfig 

#define GPIO_DAC_SS 19
#define GPIO_DAC_RESET 13
#define GPIO_SARRAY_RST 26
#define GPIO_SARRAY_CLK_CHOP 14
#define GPIO_SARRAY_MOSI 23
#define GPIO_SARRAY_SS 27
#define DAC1_CHAN 0 // DET-GATE
#define DAC2_CHAN 2 // VBB-x2
#define DAC3_CHAN 3 // NC
#define DAC4_CHAN 1 // Vref

#elif defined(FW_ID02_V1) 

#define CAM_PIN_RESET 12 // software reset will be performed
#define CAM_PIN_XCLK 2
#define CAM_PIN_SCK 18
#define CAM_PIN_MISO 23
#define CAM_PIN_MOSI 38
#define CAM_PIN_SS 13
#define CAM_PIN_D7 37
#define CAM_PIN_D6 36
#define CAM_PIN_D5 35
#define CAM_PIN_D4 34
#define CAM_PIN_D3 27
#define CAM_PIN_D2 26
#define CAM_PIN_D1 25
#define CAM_PIN_D0 19
#define CAM_PIN_VSYNC 16
#define CAM_PIN_HSYNC 33
#define CAM_PIN_HREF 32
#define CAM_PIN_PCLK 14

#define GPIO_CAMERA_RST 12
#define GPIO_CAMERA_SS 13

#define TFCARD_CS_PIN -1 // 4 // default is defined in ./components/M5Core2/src/utility/Config.h

#elif defined(FW_ID02_V2) 

//#define CONFIG_CAMERA_CORE0 1
#define CONFIG_CAMERA_DMA_BUFFER_SIZE_MAX 32768

#define CAM_PIN_PWDN -1  //power down is not used
#define CAM_PIN_SIOD -1 // use SPI in our case!
#define CAM_PIN_SIOC -1 

#define CAM_PIN_RESET 12 // software reset will be performed
#define CAM_PIN_XCLK 2
#define CAM_PIN_SCK 18
#define CAM_PIN_MISO 23
#define CAM_PIN_MOSI 38
#define CAM_PIN_SS 13
#define CAM_PIN_D7 37
#define CAM_PIN_D6 36
#define CAM_PIN_D5 35
#define CAM_PIN_D4 34
#define CAM_PIN_D3 27
#define CAM_PIN_D2 26
#define CAM_PIN_D1 25
#define CAM_PIN_D0 19
#define CAM_PIN_VSYNC 16
#define CAM_PIN_HSYNC 33
#define CAM_PIN_HREF 32
#define CAM_PIN_PCLK 14

#define GPIO_CAMERA_RST 12
#define GPIO_CAMERA_SS 13

#define TFCARD_CS_PIN 33 // default is defined in ./components/M5Core2/src/utility/Config.h

#else


// use this on original (yellow) M5 Core2 module
 // CONFIG_D0WD_PSRAM_CS_IO=16 with idf.py menuconfig 
#endif

#define MY_ROTATION 1
#define GPIO_INT_SCL 22
#define GPIO_INT_SDA 21

// ADC DEFINES
//The two address bits are loaded at the end of the present conversion sequence and select which analog input channel is
// to be converted in the next serial transfer, or they may select the final channel in a consecutive sequence as 
// described in Table 10. The selected input channel is decoded as shown in Table 7. 
// to the conversion result are also output on DOUT prior to the 12 bits of data (see the Serial Interface section). 
// The next  channel to be converted on will be selected by the mux on the 14th SCLK falling edge. 

// 12 bit CMD's
// 0bXXXX11XXXXXX
#define ADC_CMD_CH0 0b000000000000 //The two address bits are loaded at the end of the present conversion sequence and select which analog input channel is to be converted in the next serial transfer, or they may select the final channel in a consecutive sequence as described in Table 10. The selected input channel is decoded as shown in Table 7. 
#define ADC_CMD_CH1 0b000001000000
#define ADC_CMD_CH2 0b000010000000
#define ADC_CMD_CH3 0b000011000000

//0bXXXXXX11XXXX
#define ADC_CMD_PM_NORMAL 0b000000110000 // The two power management bits decode the mode of operation of the AD7904/AD7914/AD7924 as described in Table 9. 3 SEQ0 The SEQ0 bit is used in conjunction with the SEQ1 bit to control the use of the sequencer function (see Table 10). 
#define ADC_CMD_PM_FULL_SHUTDOWN 0b000000100000
#define ADC_CMD_PM_AUTO_SHUTDOWN 0b000000010000

//0bXXXXXXXXXXX1
#define ADC_CMD_CODING_2COMP 0b000000000000 // CODING This bit selects the type of output coding that the AD7904/AD7914/AD7924 will use for the conversion result. If this bit is set to 0, the output coding for the part will be twos complement. If this bit is set to 1, the output coding from the part will be straight binary (for the next conversion).
#define ADC_CMD_CODING_BIN 0b000000000001

//0bXXXXXXXXXX1X
#define ADC_CMD_RANGE_2X 0b000000000000 // RANGE This bit selects the analog input range to be used on the AD7904/AD7914/AD7924. If it is set to 0, the analog input range will extend from 0 V to 2 × REFIN. If it is set to 1, the analog input range will extend from 0 V to REFIN (for the next conversion). For the 0 V to 2 × REFIN input range, VDD = 4.75 V to 5.25 V. 
#define ADC_CMD_RANGE_1X 0b000000000010

//0bX1XXXXXX1XXX
#define ADC_CMD_SEQ_OFF 0b000000000000 // The sequencer function is not used. The analog input channel selected for each individual conversion is determined by the contents of the channel address bits, ADD1 and ADD0, in each previous write operation. This mode of operation reflects the traditional operation of a multi-channel ADC, without using the sequencer function, where each write to the AD7904/AD7914/AD7924 selects the next channel for conversion (see Figure 11).
#define ADC_CMD_SEQ_USED   0b010000000000 // The sequencer function is not interrupted upon completion of the write operation. This configuration allows other bits in the control register to be altered between conversions while in a sequence without terminating the cycle. 
#define ADC_CMD_SEQ_CONT 0b010000001000 // This configuration is used in conjunction with the channel address bits, ADD1 and ADD0, to program continuous conversions on a consecutive sequence of channels from Channel 0 to a  selected final channel that is specified by the channel address bits in the control register (see Figure 12).
//The SEQ0 bit is used in conjunction with the SEQ1 bit to control the use of the sequencer function (see Table 10).
// if this bit is set to 0, the remaining 11 bits are not loaded to the control register, which remains unchanged.  10 SEQ1 The SEQ1 bit is used in conjunction with the SEQ0 bit to control the use of the sequencer function (see Table 10).

//0b1XXXXXXX1XXX
#define ADC_CMD_WRITE 0b100000000000 // The value written to this bit determines whether the following 11 bits will be loaded to the control register. If this bit is set to 1, the following 11 bits will be written to the control register
#define ADC_CMD_SAMPLE 0b000000000000

#define DAC_SPI_Clock   10000000 // 10MHz, AD5686 DAC works up to 50MHz
#define SARRAY_SPI_Clock 1000000 // 1 MHz TCam Clock
#define ADC_SPI_Clock 10000000 // 10 MHz AD7 ADC Clock

// DAC LIMITS AND EXTERN SWEEP SETTINGS 
#define DAC_AD5689_MAX 65535 
#define DAC_AD5689_CH0_DEFAULT 41000 // pix_bias = 0.75V
#define DAC_AD5689_CH1_DEFAULT 28398 // vgate = 0.52V
#define DAC_AD5689_SWEEP_CNT 10 // limited to one byte
#define VAL_DAC_PIXBIAS_OPT 0.84
#define VAL_DAC_VGATE_OPT 0.6

// COLOR MAPS
#define GRAYSCALE 0
#define JETSCALE 1
#define WINTERSCALE 2
#define COOLSCALE 3
#define AUTUMNSCALE 4
#define SPRINGSCALE 5

// Internal Source Array tw_id01 ASIC Register
//|  7  | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
//| R/W |    CMD    |   ADDR/DATA   |
// Some CMDs require a 2nd data byte 
#define ASIC_R            0b00000000 // 0 Read ASIC register
#define ASIC_W            0b10000000 // 1 Write ASIC register
#define ASIC_CMD_CHIP_ID  0b00010000 // 1 OpCode to read Chip ID of camera array
#define ASIC_CMD_BIST_PAD 0b00100000 // 2 OpCode to enable/disable the BIST PAD
#define ASIC_CMD_SET_BIST 0b00110000 // 3 OpCode to select BIST pixel for output
#define ASIC_CMD_SET_GAIN 0b01000000 // 4 OpCode to set gain of bist opamp
#define ASIC_CMD_LOAD_MEM 0b01010000 // 5 OpCode to load the memory
#define ASIC_CMD_EN_CHOP  0b01100000 // 6 OpCode to enable/disable chopping

// Internal Camera tw_id02 ASIC Register partition
//|  7  | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
//| R/W |    CMD    |      ADDR     |
// This ASIC always requires a 2nd data byte 
#define ASIC_CAM_R         0b00000000 // 0 Read ASIC register
#define ASIC_CAM_W         0b10000000 // 1 Write ASIC register
#define ASIC_CAM_CMD         0b00010000 // 1 Write ASIC register
#define ASIC_CAM_MAX_ADDR         9 
//#define ASIC_CAMERA_CMD_MEM   0b00010000 // 1 OpCode to access memory // already or-ed into the following ADDRs
#define ASIC_CAM_CHIP_ID	         0b00010000 // 0 Address for accessing the Chip ID   
#define ASIC_CAM_CHOP_DIV  	      0b00010001 // 1 Address for accessing the ChopperDiv, b7-b0
#define ASIC_CAM_PCLK_CYC_LBYTE  	0b00010010 // 2 Address for PclkCycles, (b7-b0)
#define ASIC_CAM_PCLK_CYC_HBYTE  	0b00010011 // 3 Address for PclkCycles, (b2-b0)
#define ASIC_CAM_HSYNC_CYC  	      0b00010100 // 4 Address for HsyncCycles, (b3-b0) = How many PCLK cycles between rows, default 4
#define ASIC_CAM_HPORCH             0b00010101 // 5 Address for HFporch (b3-b0) and HBporch (b7-b4)
#define ASIC_CAM_VPORCH             0b00010110 // 6 Address for b0 is (b0)
#define ASIC_CAM_VC_EN     	      0b00010111 // 7 Address for b0 Video Controller EN
#define ASIC_CAM_SUBSTRACT_EN       0b00011000 // 8 Address for b0 Substract EN
#define ASIC_CAM_CHOPPER_SELECT     0b00011001 // 9 Address for b1-b0, bit0 ? pad or intern; bit1 ? enable output pad
#define ASIC_CAM_ROW_MIN            0b00011010 // 10 Address for b1-b0, bit0 ? pad or intern; bit1 ? enable output pad
#define ASIC_CAM_ROW_MAX            0b00011011 // 11 Address for b1-b0, bit0 ? pad or intern; bit1 ? enable output pad
#define ASIC_CAM_COL_MIN            0b00011100 // 12 Address for b1-b0, bit0 ? pad or intern; bit1 ? enable output pad
#define ASIC_CAM_COL_MAX            0b00011101 // 13 Address for b1-b0, bit0 ? pad or intern; bit1 ? enable output pad

// Camera Memory defaults (reset preloaded)
/*
uint8_t DEF_CHIP_ID   	= 0x02; 
uint8_t DEF_CHOP_DIV  	= 0x24; 
uint16_t DEF_PCLK_CYC  	= 0x0190;
uint8_t DEF_HSYNC_CYC   = 0x04;
uint8_t DEF_HPORCH		= 0x11; // Horizontal front/back porches: HFporch, HBporch
uint8_t DEF_VPORCH      = 0x01; // Vertical front/back porches: VFporch==VBporch (only 1/0)
uint8_t DEF_VC_EN     	   = 0x00;
uint8_t DEF_SUBSTRACT_EN   = 0x01;
uint8_t DEF_CHOPPER_SELECT = 0x00;
*/

#define CMD_MAX_PARAMETER_LENGTH 6 // max alowed parameters

#define MYM5_IDX_BAT 0
#define MYM5_IDX_WIFI 1
#define MYM5_IDX_VBUS 2
#define MYM5_IDX_AC 3
#define MYM5_IDX_CHARGE 4
#define MYM5_IDX_TEMP_APX192 5
#define MYM5_IDX_IPS 6
#define MYM5_IDX_RPM_FAN 7
#define MYM5_IDX_RPM_PUMP 8
#define MYM5_IDX_DATE 9

#define FEATURE_IDX_RPM 0
#define FEATURE_IDX_FILT 1
#define FEATURE_IDX_ALIAS 2
#define FEATURE_IDX_ZERO 3
#define FEATURE_IDX_SCALE 4
#define FEATURE_IDX_HOLD 5
#define FEATURE_IDX_STREAM 6
#define FEATURE_IDX_STATS 7
#define FEATURE_IDX_SERIAL 8
#define FEATURE_IDX_CAL 9
#define FEATURE_IDX_AUTOSCALE 10
#define FEATURE_IDX_DEBUG 11
#define FEATURE_IDX_WIFI 12
#define FEATURE_IDX_LDO 13
#define FEATURE_IDX_PID 14
#define FEATURE_IDX_BIST 15
#define FEATURE_IDX_ENABLE_SLIDER 16
#define FEATURE_IDX_CHOPPER 17
#define FEATURE_IDX_MONITOR 18
#define FEATURE_IDX_VPORCH 19
#define FEATURE_IDX_VC 20
#define FEATURE_IDX_SUBST 21
#define FEATURE_IDX_CHOPPER_SYNC 22
#define FEATURE_IDX_ZOOM 23
#define FEATURE_IDX_INTERPOL 24
#define FEATURE_IDX_HMIRROR 25
#define FEATURE_IDX_VMIRROR 26
#define FEATURE_IDX_ROTATE90 27
#define FEATURE_IDX_WRITE_INT16 28
#define FEATURE_IDX_PERSISTANT 29

#define IMG_ALG_TYPE_RAW 0x01
#define IMG_ALG_TYPE_OFFSET 0x02
#define IMG_ALG_TYPE_PEAK 0x04
#define IMG_ALG_TYPE_NORM 0x08
#define IMG_ALG_TYPE_FILT 0x10

#endif
