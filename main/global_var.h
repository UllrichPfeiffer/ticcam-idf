// global_var.h
#ifndef GLOBAL_VAR_H
#define GLOBAL_VAR_H

#include "AD5686.h"
#include "MCP466_DigitalPot.h" 
#include <WiFi.h>
#include "main.h"
#include "tw_class.h"
#include "BMP.h"
#include "SdFat.h"
#include <Preferences.h> // instead of #include <EEPROM.h>
#include "sdkconfig.h"

#if !(CONFIG_TWID02_SUPPORT || CONFIG_TWID03_SUPPORT || CONFIG_TWID04_SUPPORT)
    #error "You must enable one of CONFIG_TWIDxx_SUPPORT in menuconfig."
#endif

#if CONFIG_TWID04_SUPPORT
#include "Adafruit_EMC2101.h"
#include "Adafruit_MCP4725.h"
#endif

#define OR_DEFAULT 0
#define OR_HMIRROR 1
#define OR_VMIRROR 2
#define OR_HMIRROR_VMIRROR 3
#define OR_ROTATE90_RIGHT 4
#define OR_ROTATE90_LEFT 5
#define OR_VMIRROR_ROTATE90_RIGHT 6
#define AUTOSCALE_NOISE_THRESHOLD 10 // in counts
#define DEFAULT_INTEGRATION 100

#if CONFIG_TWID02_SUPPORT


#elif CONFIG_TWID03_SUPPORT


#elif CONFIG_TWID04_SUPPORT
//extern std::vector<int> doub_col_indices;
//extern static const std::unordered_set<int> doub_col_indices;
extern uint64_t doub_col_indices; 
extern MyPOT *POT_VCC_X2;
extern MyPOT *POT_VBB_X2;
extern MyPOT *POT_VCC_LNA;
extern MyPOT *POT_VCC_OSC;
extern MyADC *ADC_LNA;
extern MyADC *ADC_NTC;
extern MyADC *ADC_IMON_OSC;
extern MyADC *ADC_IMON_LNA;
extern MyM5 *MYM5_ADC_TEMP;
extern MyFeature *FEATURE_LNA_EN;
extern MyFeature *FEATURE_OSC_EN;
extern MyFeature *FEATURE_DOUB_EN;extern MyFeature *FEATURE_PAEN;
extern MyADC *ADC_TEMP;
extern MyTEMP *TEMP_SARRAY;
extern MyEMC2101 *FAN_EMC2101;
extern MyMCP4725 *DAC_MCP4725;
#endif

#if CONFIG_TWID02_SUPPORT || CONFIG_TWID03_SUPPORT
extern lv_obj_t *btnm4; // register map
extern lv_obj_t *btnm3; // peak, zero, time, histo, stats
extern MyPOT *POT_REF;
extern MyPOT *POT_DET;
extern MyPOT *POT_VCO;
extern MyADC *ADC_BIST;
//extern MyADC *ADC_TEMP;
extern MyADC *ADC_GND;
extern MyADC *ADC_VCC;
extern MyADCi2c *ADC_TEMP;
extern MyTEMP *TEMP_CAMERA;

#endif

//extern const int supportedBaudRates[];
const int supportedBaudRates[] = {9600, 19200, 38400, 57600, 115200, 250000, 500000, 1000000, 2000000};
//#define MY_ARRAY_BAUDRATE_SIZE (sizeof(supportedBaudRates) / sizeof(supportedBaudRates[0]))
extern byte XRES;
extern byte YRES;
extern int frameSize;
extern byte CANVAS_WIDTH; //should not exeed 240
extern byte CANVAS_HEIGHT; //should not exeed 240

extern Preferences preferences;
extern SdFat SD2;
extern RTC_TimeTypeDef RTCtime;
extern RTC_DateTypeDef RTCDate;
extern bool sdInitialized;
extern unsigned char bmpHeader[BMP::headerSize];
//extern unsigned char bmpHeader[54];

extern const int tcpPort; // remote port
extern unsigned int localPort; // local port to listen for UDP packets
extern lv_theme_t * th;
extern lv_theme_t * th_alarm;
extern lv_obj_t *tabview;
extern byte color_map;
extern lv_obj_t * logo;
extern lv_obj_t * table;
extern lv_obj_t * slider_color_scale_max_extern;
extern uint16_t color_scale_min;
extern uint16_t color_scale_max;
extern lv_obj_t * slider_color_scale_max;
extern lv_obj_t *roller_chopper_cyc;
extern lv_obj_t * chart;
extern lv_chart_series_t * ser;
extern lv_chart_cursor_t * cursor;
extern lv_obj_t * histo;
extern lv_chart_series_t * histo_ser;
extern int16_t chart_max;
extern int16_t histo_max;
extern int16_t chart_min;
extern lv_obj_t *canvas;

//extern AD5686 DAC_AD5686; 
extern WiFiServer server;
//extern WiFiServer server_http;
extern WiFiClient serverClients[MAX_SRV_CLIENTS];
extern WiFiClient client;
extern MCP466_DigitalPot *Pot0;
extern MCP466_DigitalPot *Pot1;

extern tw_id02 *camera;

extern ADS1100 ads;
extern uint64_t pattern;
extern uint8_t ShiftRegTx;
extern uint8_t ShiftRegRx;
extern byte cursor_row;
extern byte cursor_col;
extern uint32_t px_num;
extern uint32_t elaps;

extern lv_color_t TicWaveGreen;

void send(String message);
void my_debug(String txt);
void init_touch();
void my_touchpad_read(lv_indev_drv_t * indev_driver, lv_indev_data_t * data);
//static void btn_event_cb(lv_event_t * e);
void lv_example_get_started_1(void);
void init_display();
void my_disp_flush( lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
static void btn_off_event_handler(lv_event_t * event);
void my_timer_cb(lv_timer_t * timer);
static lv_timer_t * timer;
void WritePattern();
void ReadPattern ();
void ShowPattern();
uint8_t spi_transfer(uint8_t tx_data);
unsigned int WriteCmd(byte cmd_byte, byte data_byte);
void WriteDac(int dac_idx);
void ReadPot(int idx_pot);
void WritePot(int idx_pot);
int Float2Pot(float value, int pot_idx);
float Pot2Float(int wiper, int pot_idx);
float Dac2Float(uint16_t dac, int dac_idx);
uint16_t Float2Dac(float volt, int dac_idx);
void Val2Dac(float volt, int dac_idx);
void Cnt2Dac(uint16_t cnt, int dac_idx);
void Val2Pot(float volt, int pot_idx);
void Cnt2Pot(uint8_t cnt, int pot_idx);
uint16_t JetByteTo16bit(byte gray, byte min, byte max);
uint8_t reverse(uint8_t b);
lv_obj_t * slider_create(lv_obj_t * DAC_slider, lv_obj_t * tab, int pos, uint16_t default_val, uint16_t min_val, uint16_t max_val, const char * description);
void countupFan();
void countupPump();
void SourceToggle();
int SetupWifi();
int SetupAP_STA();
int SetupNTP();
int SetupOTA();
extern lv_color_t *cbuf;
void show_popup(lv_obj_t *parent, const char *message);
void delete_popup(lv_obj_t *parent);

//void SendFrame(uint16_t (&frame)[NUMXY][NUMXY]);
//void ReadFrame(uint16_t (&frame)[NUMXY][NUMXY], uint8_t avr_cont);
//void FillVideoBuff(uint16_t (&frame)[NUMXY][NUMXY]);
//void FillVideoBuff2(uint16_t (&frame)[NUMXY][NUMXY]);
void readFile(fs::FS &fs, char * path);
void deleteFile(fs::FS &fs, char * path);
void listDir(fs::FS &fs, const char * dirname, uint8_t levels);
static void canvas_event_handler(lv_event_t *e);
//lv_color_t ColorMap16bitRGB(byte map, uint16_t gray, uint16_t min, uint16_t max);
uint32_t ColorMap32bitRGB(byte map, uint32_t gray, uint32_t min, uint32_t max);
double goertzel_mag(int numSamples,int TARGET_FREQUENCY,int SAMPLING_RATE, double* data);

extern bool MuxSerialToWifi;
extern bool simulate;
//extern bool en_pid;
//extern bool en_bist;
extern bool en_rpm;
extern bool en_filter;
extern bool en_aliasing;
extern bool en_zero;
extern bool en_zero_save;
extern bool en_scale;
extern bool en_hold;
extern bool en_stream;
extern bool en_stats;
extern bool en_serial; // used for config via serial usb
extern bool en_cal;
// en_autoscale;
extern bool en_lvgl;  
extern bool en_debug;
extern bool en_wifi_sleep;
extern bool en_wifi;
extern bool en_log;
extern bool en_simulate;
extern bool en_source;
extern bool state_thermal_off; // indicater for FEATURE_AUTO_OFF
extern bool en_write_int16; // change data format for USB or wifi comm

extern MyCmd *cReadFile;
extern MyCmd *cSetAlarm;
extern MyCmd *cGetImage;
extern MyCmd *cSetTime;    
extern MyCmd *cSetDate;
extern MyCmd *cListDir;
extern MyCmd *cDelFile;
extern MyCmd *cGetState;
extern MyCmd *cReboot;
extern MyCmd *cSetEnDebug;
extern MyCmd *cGetStatus;
extern MyCmd *cSetEnLog;

extern MyScan *CmdReceived;
extern MyDAC *DAC_DET_GATE;
extern MyDAC *DAC_VREF;
extern MyDAC *DAC_VBB_X2;
extern MyDAC *DAC_NC;
extern MyPWM *PWM_PELTIER;
extern MyPWM *PWM_PUMP;
extern MyPWM *PWM_FAN_EXT;
extern MyCHOP *PWM_CHOP;
extern MyLDO *LDO_FAN_INT;
extern MyADC *AdcMux;
extern MyM5 *MYM5_BAT;
extern MyM5 *MYM5_WIFI;
extern MyM5 *MYM5_DATE;
extern MyM5 *MYM5_VBUS;
extern MyM5 *MYM5_AC;
extern MyM5 *MYM5_IPS;
extern MyM5 *MYM5_CHARGE;
extern MyM5 *MYM5_TEMP_APX192;
extern MyM5 *MYM5_RPM_FAN;
extern MyM5 *MYM5_RPM_PUMP;
extern MyADCESP *ADCESP_VCC_TEST;
extern MyADCESP *ADCESP_GND_TEST;
extern MyNTC *ADCESP_NTC;

extern MyMEM *MEM_PCLK_LB;
extern MyMEM *MEM_PCLK_HB;
extern MyMEM *MEM_HSYNC;
extern MyMEM *MEM_ROW_MIN_MAX;
extern MyMEM *MEM_COL_MIN_MAX;
extern MyXCLK *CAMERA_XCLK;
extern MyEXPOSER *CAMERA_EXPOSER;

extern MyFeature *FEATURE_RPM;
extern MyFeature *FEATURE_CHOP;
extern MyFeature *FEATURE_FILT;
extern MyFeature *FEATURE_ZOOM;
extern MyFeature *FEATURE_INTERPOL;
extern MyFeature *FEATURE_ALIAS;
extern MyFeature *FEATURE_ZERO;
extern MyFeature *FEATURE_SCALE;
extern MyFeature *FEATURE_HOLD;
extern MyFeature *FEATURE_STREAM;
extern MyFeature *FEATURE_STATS;
extern MyFeature *FEATURE_SERIAL;
extern MyFeature *FEATURE_CAL;
extern MyFeature *FEATURE_AUTOSCALE;
extern MyFeature *FEATURE_PERSISTANT;
extern MyFeature *FEATURE_DEBUG;
extern MyFeature *FEATURE_WIFI;
extern MyFeature *FEATURE_LDO;
extern MyFeature *FEATURE_PID;
extern MyFeature *FEATURE_BIST;
extern MyFeature *FEATURE_ENABLE_SLIDER;
extern MyFeature *FEATURE_MONITOR;
extern MyFeature *FEATURE_AUTO_OFF;
extern MyFeature *FEATURE_VPORCH;
extern MyFeature *FEATURE_VC;
extern MyFeature *FEATURE_SUBST;
extern MyFeature *FEATURE_CHOPPER_SYNC;
extern MyFeature *FEATURE_HMIRROR;
extern MyFeature *FEATURE_VMIRROR;
extern MyFeature *FEATURE_ROTATE90;
extern MyFeature *FEATURE_WRITE_INT16;

extern MyScope *SCOPE;
extern MyConfig *ThisDevice;

extern uint8_t sudo_passwd[];
static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,
    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,
    .xclk_freq_hz = 10000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
//    .pixel_format = PIXFORMAT_RGB565, //YUV422,GRAYSCALE,RGB565,JPEG
    .pixel_format = PIXFORMAT_RGB888,
    .frame_size = FRAMESIZE_33X33,    //QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.
    .row_min = 0,
    .row_max = YRES,
    .col_min = 0,
    .col_max = XRES,
    .jpeg_quality = 63, //0-63, for OV series camera sensors, lower number means higher quality
    .fb_count = 2,       //When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
                        // if set to one we will miss out every other frame!!!
    .grab_mode = CAMERA_GRAB_LATEST, //CAMERA_GRAB_LATEST, //CAMERA_GRAB_WHEN_EMPTY,
};
#endif
