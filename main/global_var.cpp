#include <M5Core2.h>
#include <lvgl.h>
//#include "global_var.h"
#include "AD5686.h"
#include "MCP466_DigitalPot.h" 
#include <WiFi.h>
#include "tw_class.h"
#include "main.h"
#include "BMP.h"
#include "sensor.h"
#include "SdFat.h"
#include <Preferences.h> // instead of #include <EEPROM.h>
Preferences preferences;

RTC_TimeTypeDef RTCtime;
RTC_DateTypeDef RTCDate;

//unsigned char bmpHeader[BMP::headerSize];
//unsigned char bmpHeader[54];

const int tcpPort = 5000; // remote port
unsigned int localPort = 5000; // local port to listen for UDP packets
bool sdInitialized = false;

#if CONFIG_TWID02_SUPPORT
byte XRES = 33;
byte YRES = 33;

#elif CONFIG_TWID03_SUPPORT
byte XRES = 64;
byte YRES = 48;

#elif CONFIG_TWID04_SUPPORT
byte XRES = 64;
byte YRES = 4;
//std::vector<int> doub_col_indices = {3, 6, 7, 11, 15, 18, 19, 23, 27, 30, 31, 35, 39, 42, 43, 47, 51, 55, 59, 63};
//static const std::unordered_set<int> doub_col_indices = {3, 6, 7, 11, 15, 18, 19, 23, 27, 30, 31, 35, 39, 42, 43, 47, 51, 55, 59, 63};
uint64_t doub_col_indices = 0b1000100010001000100011001000100011001000100011001000100011001000;
MyPOT *POT_VCC_X2;
MyPOT *POT_VBB_X2;
MyPOT *POT_VCC_LNA;
MyPOT *POT_VCC_OSC;MyADC *ADC_LNA;
MyADC *ADC_NTC;
MyADC *ADC_IMON_OSC;
MyADC *ADC_IMON_LNA;
MyM5 *MYM5_ADC_TEMP;
MyADC *ADC_TEMP;
MyTEMP *TEMP_SARRAY;
MyFeature *FEATURE_LNA_EN;
MyFeature *FEATURE_OSC_EN;
MyFeature *FEATURE_DOUB_EN;
MyFeature *FEATURE_PAEN;
MyEMC2101 *FAN_EMC2101;
MyMCP4725 *DAC_MCP4725;
#endif

#if CONFIG_TWID02_SUPPORT || CONFIG_TWID03_SUPPORT
MyPOT *POT_REF;
MyPOT *POT_DET;
MyPOT *POT_VCO;
MyADC *ADC_BIST;
//MyADC *ADC_TEMP;
MyADC *ADC_GND;
MyADC *ADC_VCC;
MyADCi2c *ADC_TEMP;
MyTEMP *TEMP_CAMERA;
#endif

lv_obj_t *btnm4; // register map
lv_obj_t *btnm3; // peak, zero, time, histo, stats

int frameSize = XRES * YRES * BYTES_PER_PIXEL;
byte CANVAS_WIDTH = (PIXEL_WIDTH * NUMX); //should not exeed 240
byte CANVAS_HEIGHT = (PIXEL_HEIGHT * NUMY); //should not exeed 240

byte color_map = GRAYSCALE;
lv_theme_t * th;
lv_theme_t * th_alarm;
lv_obj_t *tabview;

lv_obj_t * logo;
lv_obj_t * table;
lv_obj_t * slider_color_scale_max_extern;
lv_obj_t * chart;
lv_obj_t * histo;
int32_t chart_max=10;
int32_t chart_min=-10;
int32_t histo_max=50;
lv_chart_series_t * ser;
lv_chart_series_t * histo_ser;
lv_chart_cursor_t * cursor;
uint32_t color_scale_min = 0;
uint32_t color_scale_max = 255;
lv_obj_t * slider_color_scale_max;
lv_obj_t *roller_chopper_cyc;
uint64_t pattern=0;
uint8_t ShiftRegTx=0;
uint8_t ShiftRegRx=0;
lv_obj_t *canvas;
lv_color_t *cbuf = NULL;
SdFs SD2;

void listDir(fs::FS &fs, const char * dirname, uint8_t levels);
void readFile(fs::FS &fs, char * path);
void deleteFile(fs::FS &fs, char * path);

uint32_t px_num;
volatile uint32_t elaps = 0;

lv_color_t TicWavreen= lv_color_hex(0x009d8b);

// AD5686 DAC_AD5686(SPISettings(DAC_SPI_Clock, MSBFIRST, SPI_MODE1), GPIO_DAC_SS, GPIO_DAC_RESET); 

WiFiServer server(tcpPort);
//WiFiServer server_http(80);
WiFiClient serverClients[MAX_SRV_CLIENTS];
WiFiClient client;
MCP466_DigitalPot* Pot0;
MCP466_DigitalPot* Pot1;

tw_id02 *camera;

//ADS1100 ads;
//MCP466_DigitalPot Pot1(POT_ADDR1);
LV_FONT_DECLARE(my_font_name);

byte cursor_row = NUMX/2;
byte cursor_col = NUMY/2;

MyCmd *cReadFile;
MyCmd *cSetAlarm;
MyCmd *cGetImage;
MyCmd *cSetTime;    
MyCmd *cSetDate;
MyCmd *cListDir;
MyCmd *cDelFile;
MyCmd *cGetState;
MyCmd *cReboot;
MyCmd *cSetEnDebug;
MyCmd *cGetStatus;
MyCmd *cSetEnLog;

MyScan *CmdReceived;
MyDAC *DAC_DET_GATE;
MyDAC *DAC_VREF;
MyDAC *DAC_VBB_X2;
MyDAC *DAC_NC;

MyPWM *PWM_PELTIER;
MyPWM *PWM_PUMP;
MyPWM *PWM_FAN_EXT;
MyCHOP *PWM_CHOP;
MyLDO *LDO_FAN_INT;
MyADC *AdcMux;
MyM5 *MYM5_BAT;
MyM5 *MYM5_WIFI;
MyM5 *MYM5_DATE;
MyM5 *MYM5_VBUS;
MyM5 *MYM5_AC;
MyM5 *MYM5_IPS;
MyM5 *MYM5_CHARGE;
MyM5 *MYM5_TEMP_APX192;
MyM5 *MYM5_RPM_FAN;
MyM5 *MYM5_RPM_PUMP;
MyADCESP *ADCESP_VCC_TEST;
MyADCESP *ADCESP_GND_TEST;
MyNTC *ADCESP_NTC;

MyMEM *MEM_PCLK_LB;
MyMEM *MEM_PCLK_HB;
MyMEM *MEM_HSYNC;
MyMEM *MEM_ROW_MIN_MAX;
MyMEM *MEM_COL_MIN_MAX;

MyXCLK *CAMERA_XCLK;
MyEXPOSER *CAMERA_EXPOSER;

MyFeature *FEATURE_RPM;
MyFeature *FEATURE_CHOP;
MyFeature *FEATURE_FILT;
MyFeature *FEATURE_ZOOM;
MyFeature *FEATURE_INTERPOL;
MyFeature *FEATURE_ALIAS;
MyFeature *FEATURE_ZERO;
MyFeature *FEATURE_SCALE;
MyFeature *FEATURE_HOLD;
MyFeature *FEATURE_STREAM;
MyFeature *FEATURE_STATS;
MyFeature *FEATURE_SERIAL;
MyFeature *FEATURE_CAL;
MyFeature *FEATURE_AUTOSCALE;
MyFeature *FEATURE_PERSISTANT;
MyFeature *FEATURE_DEBUG;
MyFeature *FEATURE_WIFI;
MyFeature *FEATURE_LDO;
MyFeature *FEATURE_BIST;
MyFeature *FEATURE_ENABLE_SLIDER;
MyFeature *FEATURE_MONITOR;
MyFeature *FEATURE_VPORCH;
MyFeature *FEATURE_VC;
MyFeature *FEATURE_SUBST;
MyFeature *FEATURE_CHOPPER_SYNC;
MyFeature *FEATURE_HMIRROR;
MyFeature *FEATURE_VMIRROR;
MyFeature *FEATURE_ROTATE90;
MyFeature *FEATURE_WRITE_INT16;
MyConfig *ThisDevice;
MyScope *SCOPE;

bool simulate = false;
//bool en_pid = false;
//bool en_bist = false;
bool en_rpm = false;
bool en_filter = false;
bool en_aliasing = false;
bool en_zero = true; // now always true at startup because we now load zero frame from NVS 
bool en_zero_save = false;
bool en_scale = false;
bool en_hold = false;
bool en_stream = false;
bool en_stats = true;
bool en_serial = true; // used for config via serial usb
bool en_cal = true;
// en_autoscale = false;
bool en_lvgl = false;  
bool en_debug = true; // this can be overwritten with DoItEnDebug cmd in tw_class.cpp 
bool en_wifi_sleep = true;
bool en_wifi = false;
bool en_log = false;
bool en_simulate = true;
bool en_source = false;
bool state_thermal_off = false; // indicater for FEATURE_AUTO_OFF
bool en_write_int16 = true;

// echo -n "ticwave" | sha256sum
// uint8_t sudo_passwd[] = {0xf9, 0xd4, 0x77, 0x0f, 0x5c, 0xf9, 0x6c, 0x62, 0xf1, 0x1f, 0x09, 0xd0, 0x36, 0xd3, 0x26, 0x11, 0xee, 0x58, 0x34, 0x22, 0xf4, 0x5b, 0x52, 0x73, 0x9a, 0x64, 0x19, 0x00, 0x10, 0xf3, 0x99, 0x2e};
//echo -n "tic4wave" | sha256sum
uint8_t sudo_passwd[] = {0x9a, 0x18, 0xc3, 0x5e, 0x95, 0x78, 0x5e, 0xb3, 0x98, 0xfb, 0xea, 0xfc, 0x86, 0x8f, 0x40, 0x73, 0xf3, 0x16, 0xd4, 0x50, 0x56, 0x52, 0x8b, 0xf2, 0xa5, 0xce, 0x9d, 0x34, 0xf3, 0x33, 0x15, 0xf0};
