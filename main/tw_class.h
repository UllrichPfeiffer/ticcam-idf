// TicWave GmbH, U. Pfeiffer 15.3.2023
// Header related to the serial communication
#pragma once
#include <lvgl.h>
#include <vector>
#include "MCP466_DigitalPot.h" 
#include "ADS1100.h"
#include "XClk.h"
#include "driver/ledc.h"
#include "Log.h"
#include "esp_camera.h"
#include "SdFat.h"
#include "main.h"

using namespace std;

/*********************************/
/* Serial/Wifi Command CLASS DEF */
/*********************************/

class MyCmd{
  public:    
    MyCmd(String cmd_, char cmd_char_, String txt_, uint8_t num_par_, String delim_, void (*DoIt_)(MyCmd *obj)) ; // if num_par_ = 0 parameter is interprated as String    
    String cmd; // command short name (used e.g. from a command line script)
    char cmd_char; // data to be transmitted via Serial
    String txt; // command description, used for help functions
    String delim; // delimiter to use for scanning the token
    double *param; // pointer to parameters
    uint8_t num_par; // number of command parameters
    String param_txt; // A text parameter, e.g. a filename if num_par=0
    void SerialPrint(); // Serial print all parameters of the command
    void ScanToken(String token); // Scans a token for the parameters
    void (*DoIt)(MyCmd *obj); 
};

/*********************************/
/* Cam/Source Config   CLASS DEF */
/*********************************/

//#define NUM_REMOTE_CMDS_PUBLIC 5
//#define NUM_REMOTE_CMDS_PRIVATE 2


class MyConfig{
  public:    
    MyConfig(String firmware_, String revision_, String wifi_ssid_, String wifi_passwd_); // if num_par_ = 0 parameter is interprated as String    
    char cmd; // serial command
    String firmware;
    String revision;
    String wifi_ssid;
    String wifi_passwd;  
    uint32_t baudrate;
    cJSON *cJSON_info; // destructor: cJSON_Delete(cJSON_info);
    cJSON *cJSON_cmdlist; // destructor: cJSON_Delete(cJSON_cmdlist);
    bool sudo;
    uint8_t chip_id;
    uint8_t num_remote_cmds_public;
    uint8_t num_remote_cmds_private;
  //  MyCmd *RemoteCmdsPublic;
    vector<MyCmd> RemoteCmdsPublic;
    vector<MyCmd> RemoteCmdsPrivate;    
    void SerialPrint();
    void SendJsonCmdInfo();
    void SerialInfo();
};

/*********************************/
/* Serial/Wifi Scanner CLASS DEF */
/*********************************/

class MyScan {      
  public:    
    MyScan(String delim_);        
    char cmd; // serial command received
    String token; // part following the command code
    String delim;
    void SplitLine(String line);
    void DoIt();
    void SerialPrint();
};

/********************/
/* DEVICE CLASS DEF */
/********************/

class MyDevice {      
  public:    
    MyDevice(String txt_, String fmt_, uint16_t cnt_, uint16_t cnt_min_, uint16_t cnt_max_, float norm_);        
    ~MyDevice();
    String txt; 
    String fmt;
    uint16_t cnt;
    uint16_t cnt2;
    uint16_t cnt_min;
    uint16_t cnt_max;
    uint8_t tab_idx;
    float norm;
    lv_obj_t * table;
    lv_obj_t * slider;
    lv_obj_t * slider_label;
    lv_obj_t * info_txt;
    lv_slider_mode_t slider_mode;
    void UpdateTable(); 
    void UpdateSlider();
    void HideSlider();
    void UnHideSlider();
    void SerialPrint();
    void SliderCreate(int posy, lv_slider_mode_t mode, lv_obj_t *obj);
    void TableCreate(int tab_idx, lv_obj_t * table);
    virtual float Cnt2Volt(uint16_t cnt);
    uint16_t Volt2Cnt(float volt);
    static void SliderCB(lv_event_t * event);
    virtual void DoIt();  // this is where the action happens, redefined in child class
};

/*****************/
/* DAC CLASS DEF */
/*****************/  
class MyDAC: public MyDevice {
  public:   
    MyDAC(uint8_t dac_chan_, String txt_, String fmt_, uint16_t cnt_, uint16_t cnt_min_, uint16_t cnt_max_, float norm_);
    ~MyDAC();
    uint8_t dac_chan; 
    void DoIt();  
};

/*****************/
/* POT CLASS DEF */
/*****************/

class MyPOT: public MyDevice {
  public:   
    MyPOT(MCP466_DigitalPot* pot_, uint8_t pot_chan_, String txt_, String fmt_, uint16_t cnt_, uint16_t cnt_min_, uint16_t cnt_max_, float norm_);
    ~MyPOT();
    uint8_t pot_chan; 
    MCP466_DigitalPot* pot;
    void DoIt();
};

/*****************/
/* PWM CLASS DEF */
/*****************/

class MyPWM: public MyDevice {
  public:   
    MyPWM(uint8_t pwm_pin_, uint8_t pwm_chan_, uint16_t pwm_freq_, String txt_, String fmt_, uint16_t cnt_, uint16_t cnt_min_, uint16_t cnt_max_, float norm_);
    ~MyPWM();
    uint8_t pwm_pin; 
    uint8_t pwm_chan; 
    uint16_t pwm_freq; 
    virtual void DoIt();
    void DetachPin();
    void AttachPin();
};

/*********************/
/* CHOPPER CLASS DEF */
/*********************/

class MyCHOP: public MyPWM {
  public:   
    MyCHOP(uint8_t pwm_pin_, uint8_t pwm_chan_, uint16_t pwm_freq_, String txt_, String fmt_, uint16_t cnt_, uint16_t cnt_min_, uint16_t cnt_max_, float norm_);
    ~MyCHOP();
    void DoIt();
};

/*****************/
/* LDO CLASS DEF */
/*****************/

class MyLDO: public MyDevice {
  public:   
    MyLDO(uint8_t ldo_chan_, String txt_, String fmt_, uint16_t cnt_, uint16_t cnt_min_, uint16_t cnt_max_, float norm_);
    ~MyLDO();
    uint8_t ldo_chan; 
    void DoIt();
};

/*****************/
/* ADC CLASS DEF */
/*****************/

class MyADC: public MyDevice {
  public:   
    MyADC(uint16_t adc_chan_, uint16_t tx_data_, String txt_, String fmt_, uint16_t cnt_max_, float norm_);
    ~MyADC();
    uint16_t tx_data;
    uint16_t adc_chan;
    uint32_t rx_data;
    void DoIt();
    void DoItFast();
    void BeginTransaction();
    void EndTransaction();
};

/*****************/
/* ADCi2c CLASS DEF */
/*****************/

class MyADCi2c: public MyDevice {
  public:   
    MyADCi2c(uint8_t i2cAddress, String txt_, String fmt_, uint16_t cnt_max_, float norm_);
    ~MyADCi2c();
    void DoIt();    
    ADS1100* my_ads;
};

/***********************/
/* ADC ESP32 CLASS DEF */
/***********************/

class MyADCESP: public MyDevice {
  public:   
    uint8_t gpio_adc; 
    MyADCESP(uint8_t gpio_adc_, String txt_, String fmt_, uint16_t cnt_max_, float norm_);
    void DoIt();
};

/*****************/
/* NTC CLASS DEF */
/*****************/
class MyNTC: public MyDevice {
  public:   
    uint8_t gpio_adc; 
    MyNTC(uint8_t gpio_adc_, String txt_, String fmt_, uint16_t cnt_max_, float norm_);
    void DoIt();
    float Cnt2Volt(uint16_t my_cnt);
};

/**************************/
/*  TEMP Sensor CLASS DEF */
/**************************/
class MyTEMP: public MyDevice {
  public:   
    MyTEMP(String txt_, String fmt_, float norm_);
    void DoIt();
    float Cnt2Volt(uint16_t my_cnt);
};

/******************/
/* MYM5 CLASS DEF */
/******************/

class MyM5{
  public:   
    MyM5(uint8_t dev_idx_, String txt_, String fmt_);
    ~MyM5(){};
    String txt;
    String fmt;
    String symbol;
    uint8_t dev_idx; 
    uint8_t tab_idx;
    lv_obj_t *table;
    float voltage;
    float current;
    float power;
//    float level;
    float percentage;
    void DoIt();
    void TableCreate(int tab_idx_, lv_obj_t * table_);
    void UpdateTable(); 
    void SerialPrint();
};

/*********************/
/* Feature CLASS DEF */
/*********************/

class MyFeature{
  public:   
    MyFeature(uint8_t feature_idx_, String txt_, bool state_);
    ~MyFeature(){};
    uint8_t feature_idx; 
    String txt;
    bool state;
    String symbol;
    lv_obj_t *checkbox; 
    static void cb(lv_event_t * event);
    void Update(); 
    void SerialPrint();
    virtual void DoIt();  // this is where the action happens
    void CheckboxCreate(int posy, lv_obj_t * obj);
    void Hide();
    void UnHide();
};

/*********************/
/* MyGraph CLASS DEF */
/*********************/

class MyGraph{
  public:   
    MyGraph(uint8_t graph_idx_, String txt_, lv_obj_t *canvas_,  MyADC *adc_);
    ~MyGraph(){};
    uint8_t graph_idx; 
    String txt;
    lv_obj_t *canvas;
    MyADC *adc;
    uint16_t cnt;
//    void Init();
//    virtual void DoIt();  // this is where the action happens
};

/*********************/
/* MyScope CLASS DEF */
/*********************/

class MyScope: public MyGraph {
  public:   
    MyScope(uint8_t graph_idx_, String txt_, lv_obj_t *canvas_,  MyADC *adc_);
// for time, freq, and spectrogram:
    lv_draw_line_dsc_t line_dsc;
    lv_draw_label_dsc_t label_dsc;
    static void cb(lv_event_t * event); // generic call back, not used yet
    void DoIt();
    void TimeDomain();
    void Histogram(uint8_t type);
    void Spectrogram();
    uint32_t bilinearInterpolation(uint32_t topLeft, uint32_t topRight, uint32_t bottomLeft, uint32_t bottomRight, double x, double y);
    lv_color_t bilinearInterpolationRGB(lv_color_t tl, lv_color_t tr, lv_color_t bl, lv_color_t br, double x_ratio, double y_ratio);
// video part:
    lv_color_t color;
    uint32_t min; 
    uint32_t max;
    uint8_t max_idx_col;
    uint8_t max_idx_row;
    uint8_t shift;
    uint8_t avr_cnt;
    uint8_t img_alg_type;
    float mean;
    float stdev;
    bool avr_type;
    lv_draw_rect_dsc_t rect_dsc;
    void Send(camera_fb_t *fb); // send data to serial port
    //void Save(camera_fb_t *fb, fs::File csv);
    void Save(camera_fb_t *fb, FsFile &csv);
    void Stream(camera_fb_t *fb);
    void AddToPersistance(camera_fb_t *fb);
    esp_err_t Video(); // show video
    camera_fb_t *FbGet(uint8_t orientation);
//    esp_err_t ZeroFrame(); // take averaged zero frame
//    esp_err_t PeakFrame(); // take averaged peak frame
    esp_err_t BackupFrame(uint8_t *buf); // take an image and back it up, e.g for zero data
    esp_err_t ImageProcessing(camera_fb_t *fb, uint8_t *my_offset, uint8_t *my_peak, bool convert2rgb); // e.g. scale pixel and filter a frame
    esp_err_t StatsOnFrame(uint8_t *buf); // get statistics on frame
    esp_err_t SaveFrame(uint8_t *buf); // in NVS memory
    esp_err_t LoadFrame(uint8_t *buf); // from NVS memory
    uint8_t offset[NUMX*NUMY*4];
    uint8_t peak[NUMX*NUMY*4];
    uint8_t persistance[NUMX*NUMY*4];
};

/******************/
/* SPI  CLASS DEF */
/******************/

class MySPI{
  private:
    uint8_t GPIO_RST;
    uint8_t GPIO_SS;
    uint32_t spi_clock;
    uint8_t bit_order;
    uint8_t data_mode;

  public:   
    MySPI(uint8_t GPIO_RST_, uint8_t GPIO_SS_, uint32_t SPI_Clock_, uint8_t BIT_ORDER_, uint8_t DATA_MODE_);
    MySPI(uint8_t GPIO_RST_, uint8_t GPIO_SS_);
    uint8_t tx_data;
    uint8_t rx_data;
    void Reset();
    void DoIt();
    void DoItFast();
    void BeginTransaction();
    void EndTransaction();
    void WriteMem(uint8_t addr, uint8_t data);
    uint8_t ReadMem(uint8_t addr);
};

/*********************/
/* tw_id02 CLASS DEF */
/*********************/

class tw_id02: public MySPI
{
  public:
  int xres;
  int yres;

  protected:
  static const int ADDR = 0x00;
  
  //spiclass spi;

  public:
  void initialize(); 
  void testImage();
  int init();
  uint8_t Col2ChopperDiv(uint8_t col);
  uint8_t Col2Porch(uint8_t col);

  public:
  tw_id02(const int xres, const int yres, const int CAM_RESET, const int VSYNC, const int HREF, uint8_t XCLK, const int PCLK, const int D0, const int D1, const int D2, const int D3, const int D4, const int D5, const int D6, const int D7);

// get it from findFPORCH script at /home/ullrich/src/OpenLane/designs/tw_asic_id02_camera/tw_scripts
// CYC_CHOPPER, CHOPPER_DIV, PORCH=FPORCH=BPORCH, HSYNC_CYC


    #if CONFIG_TWID02_SUPPORT
const uint8_t lookupValidChopper[11][4] = {
{1, 70, 1, 2}, // deadtime=2.86%
{2, 36, 2, 2}, // deadtime=2.78%
{3, 24, 2, 2}, // deadtime=2.78%
{4, 18, 2, 2}, // deadtime=2.78%
{5, 14, 1, 2}, // deadtime=2.86%
{6, 12, 2, 2}, // deadtime=2.78%
{7, 10, 1, 2}, // deadtime=2.86%
{9, 8, 2, 2}, // deadtime=2.78%
{12, 6, 2, 2}, // deadtime=2.78%
{18, 4, 2, 2}, // deadtime=2.78%
{35, 2, 1, 2}, // deadtime=2.86%
};

    #elif CONFIG_TWID03_SUPPORT
const uint8_t lookupValidChopper[16][4] = {
{1, 132, 1, 2}, // deadtime=1.52%
{2, 66, 1, 2}, // deadtime=1.52%
{3, 44, 1, 2}, // deadtime=1.52%
{4, 40, 15, 2}, // deadtime=1.25%
{5, 32, 15, 2}, // deadtime=1.25%
{6, 22, 1, 2}, // deadtime=1.52%
{7, 20, 5, 2}, // deadtime=1.43%
{8, 18, 7, 2}, // deadtime=1.39%
{9, 16, 7, 2}, // deadtime=1.39%
{10, 14, 5, 2}, // deadtime=1.43%
{11, 12, 1, 2}, // deadtime=1.52%
{14, 10, 5, 2}, // deadtime=1.43%
{17, 8, 3, 2}, // deadtime=1.47%
{22, 6, 1, 2}, // deadtime=1.52%
{33, 4, 1, 2}, // deadtime=1.52%
{66, 2, 1, 2}, // deadtime=1.52%
};

    #elif CONFIG_TWID04_SUPPORT
const uint8_t lookupValidChopper[16][4] = {
{1, 132, 1, 2}, // deadtime=1.52%
{2, 66, 1, 2}, // deadtime=1.52%
{3, 44, 1, 2}, // deadtime=1.52%
{4, 40, 15, 2}, // deadtime=1.25%
{5, 32, 15, 2}, // deadtime=1.25%
{6, 22, 1, 2}, // deadtime=1.52%
{7, 20, 5, 2}, // deadtime=1.43%
{8, 18, 7, 2}, // deadtime=1.39%
{9, 16, 7, 2}, // deadtime=1.39%
{10, 14, 5, 2}, // deadtime=1.43%
{11, 12, 1, 2}, // deadtime=1.52%
{14, 10, 5, 2}, // deadtime=1.43%
{17, 8, 3, 2}, // deadtime=1.47%
{22, 6, 1, 2}, // deadtime=1.52%
{33, 4, 1, 2}, // deadtime=1.52%
{66, 2, 1, 2}, // deadtime=1.52%
};
    #else
    #endif

/* use script /home/ullrich/src/OpenLane/designs/tw_asic_id02_camera/tw_scripts/findFPORCH
to find valid settings for non-beating chopper cycles (CYC_CHOPPER)

CHOPPER_DIV: 2 (x2) CYC_CHOPPER: 36 FPORCH=BPORCH: 1 (x1)
CHOPPER_DIV: 4 (x4) CYC_CHOPPER: 18 FPORCH=BPORCH: 1 (x1)
CHOPPER_DIV: 6 (x6) CYC_CHOPPER: 12 FPORCH=BPORCH: 1 (x1)
CHOPPER_DIV: 8 (x8) CYC_CHOPPER: 9 FPORCH=BPORCH: 1 (x1)
CHOPPER_DIV: 10 (xA) CYC_CHOPPER: 8 FPORCH=BPORCH: 5 (x5)
CHOPPER_DIV: 12 (xC) CYC_CHOPPER: 6 FPORCH=BPORCH: 1 (x1)
CHOPPER_DIV: 16 (x10) CYC_CHOPPER: 5 FPORCH=BPORCH: 5 (x5)
CHOPPER_DIV: 18 (x12) CYC_CHOPPER: 4 FPORCH=BPORCH: 1 (x1)
CHOPPER_DIV: 24 (x18) CYC_CHOPPER: 3 FPORCH=BPORCH: 1 (x1)
CHOPPER_DIV: 36 (x24) CYC_CHOPPER: 2 FPORCH=BPORCH: 1 (x1)
CHOPPER_DIV: 72 (x48) CYC_CHOPPER: 1 FPORCH=BPORCH: 1 (x1)
*/
};

/*****************/
/* MEM CLASS DEF */
/*****************/  
class MyMEM: public MyDevice {
  public:   
    MyMEM(uint8_t address, String txt_, String fmt_, uint16_t cnt_, uint16_t cnt_min_, uint16_t cnt_max_, float norm_);
    ~MyMEM();
    uint8_t address; 
    void DoIt();  
};

/*********************/
/* EXPOSER CLASS DEF */
/*********************/  
class MyEXPOSER: public MyDevice {
  public:   
    MyEXPOSER(String txt_, String fmt_, uint16_t cnt_, uint16_t cnt_min_, uint16_t cnt_max_, float norm_);
    ~MyEXPOSER();
    void DoIt();  
};

/*****************/
/* XCLK CLASS DEF */
/*****************/  

class MyXCLK: public MyDevice {
  public:   
    MyXCLK(uint8_t pwm_pin_, uint8_t pwm_chan_, String txt_, String fmt_, uint16_t cnt_, uint16_t cnt_min_, uint16_t cnt_max_, float norm_);
    ~MyXCLK();
    uint8_t pwm_pin; 
    uint8_t pwm_chan; 
    int pwm_freq; 
    ledc_timer_config_t timer_conf;
    ledc_channel_config_t ch_conf;
    virtual void DoIt();
    void ClockEnable();
    void ClockDisable();
};
