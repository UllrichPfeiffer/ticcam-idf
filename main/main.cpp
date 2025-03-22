// TicWave GmbH, U. Pfeiffer 15.3.2023
// Main code for M5StackSarray_idf_v1
#pragma GCC optimize("Ofast")

#include "sensor.h"
#include <M5Core2.h>
#include <WiFi.h>
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Ticker.h>
#include <lvgl.h>
#include "../components/lvgl/src/font/lv_font_montserrat_22.c"
#include "../components/lvgl/src/font/lv_font.h"
#include "cam_filter.h"
#include "colormap.h"
#include "MCP466_DigitalPot.h"
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <esp_sntp.h>
#include "esp_log.h"
#include "driver/i2s.h"
#include "driver/ledc.h"
#include "esp_http_server.h"
#include "main.h"
#include "global_var.h"
#include "tw_class.h" // TicWave class defs for serial IO
#include "TicWaveLogo6_98x30_g.c" // on COLOR_SCR // https://lvgl.io/tools/imageconverter
#include "SdFat.h"

    #if CONFIG_TWID02_SUPPORT
#include "camera_index_id02.h"
    #elif CONFIG_TWID03_SUPPORT
#include "camera_index_id03.h"
    #elif CONFIG_TWID04_SUPPORT
#include "camera_index_id04.h"
    #else
#error "No valid CONFIG_TWIDxx_SUPPORT defined!"
    #endif

static esp_err_t http_server_init();
uint16_t tab_count;

unsigned char bmpHeader[BMP::headerSize];

typedef struct
{
    size_t size;  //number of values used for filtering
    size_t index; //current value index
    size_t count; //value count
    int sum;
    int *values; //array to be filled with values
} ra_filter_t;

float POT_OSC_v = 1.35;       // Osc supply in volt // 1.4
float POT_IREF_v = 1.05;      // doubler supply in volt: do not bias below VBB-X2 // 1.0
float POT_VCC_roVCO_v = 1.30; // 1.2
float POT_Vmirror_v = 0.95;   // 1.0
lv_color_t TicWaveGreen = lv_color_hex(0x009d8b);

static lv_disp_draw_buf_t draw_buf;
static lv_color_t b1[(LCD_WIDTH * LCD_HEIGHT) / 10];
static lv_color_t b2[(LCD_WIDTH * LCD_HEIGHT) / 10];
static lv_obj_t *img;
static void btnm3_event_handler(lv_event_t *e);
static void btnm4_event_handler(lv_event_t *e);
static void roller_CMD_event_handler(lv_event_t *e);
static void roller_RW_event_handler(lv_event_t *e);
static void slider_color_scale_max_event_cb(lv_event_t *e);
static void roller_colormap_event_handler(lv_event_t *e);
static void roller_chopper_cyc_handler(lv_event_t *e);
static void roller_avr_event_handler(lv_event_t *e);
static void chart_event_cb(lv_event_t * e);
lv_obj_t *roller_avr;
lv_obj_t *roller_colormap;
bool MuxSerialToWifi = false;
bool stringComplete = false; // whether the string is complete

String currentLine = ""; // a String to hold incoming data

static const char *btnm_map_ctrl2[] = {LV_SYMBOL_LINEUP, "\n",
                                       LV_SYMBOL_LINEDOWN, "\n",
                                       LV_SYMBOL_SQUAREWAVE, "\n",
                                       LV_SYMBOL_HISTO, "\n",
                                       LV_SYMBOL_CALCULATOR, ""};

/*
    LV_SYMBOL_SQUAREWAVE,
    LV_SYMBOL_BEAT,
    LV_SYMBOL_ZERO,
    LV_SYMBOL_SLIDER,
    LV_SYMBOL_CALCULATOR,
    LV_SYMBOL_EXPAND,
    LV_SYMBOL_ICE,
    LV_SYMBOL_THERMO5,
    LV_SYMBOL_THERMO4,
    LV_SYMBOL_THERMO3,
    LV_SYMBOL_THERMO2,
    LV_SYMBOL_THERMO1,
    LV_SYMBOL_CHIP,
    LV_SYMBOL_CHART,
    LV_SYMBOL_LINEDOWN,
    LV_SYMBOL_LINEUP,
    LV_SYMBOL_ARRAY,
    LV_SYMBOL_BULBOFF,
    LV_SYMBOL_BULBON,
*/

void my_debug(String txt)
{
  if (en_debug) ESP_LOGE("TICWAVE", "%s", (const char *)txt.c_str());
  //  ESP_LOGE("TAG", "Error message");
}

int menue_task = 0;
struct tm timeinfo;

/*
  const char *bat_symbol = LV_SYMBOL_BATTERY_EMPTY;
  const char *wifi_symbol = LV_SYMBOL_WIFI;
  const char *powerVBUS_symbol = LV_SYMBOL_USB;
  const char *powerAC_symbol = LV_SYMBOL_RIGHT;
  const char *charge_symbol = LV_SYMBOL_CHARGE;
  const char *color_symbol = "#0000";
*/

/**********************
 *      TYPEDEFS
 **********************/
typedef struct
{
  uint32_t perf_last_time;
  uint32_t elaps_sum;
  uint32_t frame_cnt;
  uint32_t fps_sum_cnt;
  uint32_t fps_sum_all;
  uint16_t cnt;
  lv_obj_t *perf_label;
} perf_monitor_t;

static perf_monitor_t perf_monitor;

static void perf_monitor_init(perf_monitor_t *_perf_monitor)
{
  LV_ASSERT_NULL(_perf_monitor);
  _perf_monitor->elaps_sum = 0;
  _perf_monitor->fps_sum_all = 0;
  _perf_monitor->fps_sum_cnt = 0;
  _perf_monitor->frame_cnt = 0;
  _perf_monitor->perf_last_time = 0;
  _perf_monitor->perf_label = NULL;
  _perf_monitor->cnt = 0;
}

//=====================================================================
void setup()
{

  ThisDevice = new MyConfig(FIRMWARE, REVISION, WIFI_SSID_AP, WIFI_PASSWD);
  Serial.begin(DEFAULT_SERIAL_BAUDRATE);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB
  }
  Serial.flush();  // Clear the serial receive buffer

  if(en_debug) esp_log_level_set("*", ESP_LOG_VERBOSE);
  else {
    Serial.setDebugOutput(false);
    esp_log_level_set("*", ESP_LOG_NONE);
  }

  my_debug("start setup");
  // Init
  int n_elements = 1000;
  if (0)
  {
    int n_elements = 1000;
    log_d("Total heap: %d", ESP.getHeapSize());
    log_d("Free heap: %d", ESP.getFreeHeap());
    log_d("Total PSRAM: %d", ESP.getPsramSize());
    log_d("Free PSRAM: %d", ESP.getFreePsram());

    if (psramInit())
    {
      my_debug("\nPSRAM is correctly initialized");
    }
    else
    {
      my_debug("PSRAM not available");
    }

    // Create an array of n_elements
    int available_PSRAM_size = ESP.getFreePsram();
    my_debug((String) "PSRAM Size available (bytes): " + available_PSRAM_size);

    int *array_int = (int *)ps_malloc(n_elements * sizeof(int)); // Create an integer array of n_elements
    array_int[0] = 42;
    array_int[999] = 42; // We access array values like classic array

    int available_PSRAM_size_after = ESP.getFreePsram();
    my_debug((String) "PSRAM Size available (bytes): " + available_PSRAM_size_after); // Free memory space has decreased
    int array_size = available_PSRAM_size - available_PSRAM_size_after;
    my_debug((String) "Array size in PSRAM in bytes: " + array_size);

    // Delete array
    free(array_int); // The allocated memory is freed.
    my_debug((String) "PSRAM Size available (bytes): " + ESP.getFreePsram());

    // static lv_color_t cbuf[LV_CANVAS_BUF_SIZE_TRUE_COLOR(CANVAS_WIDTH, CANVAS_WIDTH)];
    // lv_color_t *cbuf;
    my_debug((String) "PSRAM Size available before (bytes): " + ESP.getFreePsram());
    my_debug((String) "PSRAM Size available after (bytes): " + ESP.getFreePsram());
  }

  //  M5.begin(true, true, true, true, kMBusModeInput); // enable/disable LCD, TF Card, Serial port and I2C port, Mbus supply ext on 5V bus pin
  M5.begin(true, false, true, true, kMBusModeOutput); // enable/disable LCD, TF Card, Serial port and I2C port, Mbus supply ext on 5V bus pin
                                                      // !!! you need to free I2S0 resources blocked by the speaker: uncomment Spk.begin(); in M5Core2.cpp

  Serial.begin(DEFAULT_SERIAL_BAUDRATE); // need to reset the serial baudrate

  M5.Axp.SetSpkEnable(0);
  M5.Axp.SetLed(1);
  M5.Axp.SetLcdVoltage(2800);
  M5.Axp.SetLcdVoltage(3300);
  M5.Lcd.begin();
  M5.Lcd.setRotation(MY_ROTATION);

  M5.Lcd.println("Booting TicWave " + ThisDevice->firmware);
  my_debug("Booting TicWave " + ThisDevice->firmware);
  
#if CONFIG_TWID04_SUPPORT
  M5.Axp.SetBusPowerMode(1); // do not switch off ext 5V bus pin
#else
  M5.Axp.SetBusPowerMode(0); // set 0 for USB/BAT power supply, set 1 for external input power supply and change kMBusModeOutput in M5.begin
#endif
  M5.Axp.SetCHGCurrent(AXP192::kCHG_550mA);

  if(0){
  my_debug("TFCARD_CS_PIN: " + String(TFCARD_CS_PIN));
  my_debug("TFCARD_: " + String(MISO));
  M5.Lcd.println("- Mounting SD-card"); 
  my_debug("- Mounting SD-card");
  if(!SD.begin()){
    M5.Lcd.println("Card Mount Failed");
    sdInitialized = false;
  } else {
    M5.Lcd.println("SD Card Initialized.");
    sdInitialized = true;
  }
  }
  M5.Lcd.println("- Mounting SPIFFS");
  my_debug("- Mounting SPIFFS");
  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
  {
    M5.Lcd.println("  SPIFFS Mount Failed");
    // return;
  }
  else
  {
    M5.Lcd.println("  SPIFFS Mounted");
  }

  M5.Lcd.println("- Configure GPIOs");

  pinMode(GPIO_CAMERA_SS, OUTPUT);
  pinMode(CAM_PIN_RESET, OUTPUT);     // set reset pin to output
  digitalWrite(GPIO_CAMERA_SS, HIGH); // make sure it is not selected
  digitalWrite(CAM_PIN_RESET, HIGH);  // no reset yet

  /*
  pinMode(GPIO_DAC_SS, OUTPUT);
  digitalWrite(GPIO_DAC_SS, HIGH); // make sure DAC is not selected
  pinMode(GPIO_ADC_SS, OUTPUT);
  digitalWrite(GPIO_ADC_SS, HIGH); // is low active

  pinMode(GPIO_FAN_RPM, INPUT);
  pinMode(GPIO_PUMP_RPM, INPUT);

   pinMode (GPIO_TEST_VCC, INPUT);
   pinMode (GPIO_TEST_GND, INPUT);
   pinMode (GPIO_NTC, INPUT);
*/

  M5.Lcd.println("- Configure Camera");

  // SPI.begin(CAM_PIN_SCK, CAM_PIN_MISO, CAM_PIN_MOSI, CAM_PIN_SS);
  camera = new tw_id02(XRES, YRES, CAM_PIN_RESET, CAM_PIN_VSYNC, CAM_PIN_HREF, CAM_PIN_XCLK, CAM_PIN_PCLK, CAM_PIN_D0, CAM_PIN_D1, CAM_PIN_D2, CAM_PIN_D3, CAM_PIN_D4, CAM_PIN_D5, CAM_PIN_D6, CAM_PIN_D7);
  
  BMP::construct16BitHeader(bmpHeader, camera->xres, camera->yres);
  
//  BMP::construct16BitHeader(bmpHeader, camera->xres, camera->yres);

  // read Chip ID
  //camera->ReadMem(ASIC_CAM_CHIP_ID);
  //Serial.printf("Chip ID read=%d\n", camera->rx_data);

/*
  // reset cam
  digitalWrite(CAM_PIN_RESET, LOW); // reset is low active
  delay(2);
  digitalWrite(CAM_PIN_RESET, HIGH); // apply reset (async reset part)


  // chip ID
  camera->tx_data = 0;
  camera->DoIt();
  Serial.printf("Chip ID garbage=%d\n", camera->rx_data);

  // Enable Video Controller
  camera->WriteMem(ASIC_CAM_VC_EN, 1);
  // read back
  camera->ReadMem(ASIC_CAM_VC_EN);
  Serial.printf("VC enabled?: %d\n", camera->rx_data);
*/

  // ++++++ Used for internal chopping signal ++++++

  #ifdef CONFIG_LDOFAN_ENABLE
    M5.Axp.SetLDOEnable(3, true);
    M5.Axp.SetLDOVoltage(3, 2500);
  #else
    M5.Axp.SetLDOEnable(3, false);
  #endif

  M5.Lcd.println("- Init POT");
  // Pot.setWireInterface(&Wire1);
  // Pot.mcpWrite(TCON, 0xFF);
  // delay(100);
  Pot0 = new MCP466_DigitalPot(POT_ADDR);
  Pot0->setWireInterface(&Wire1);
  Pot0->mcpWrite(TCON, 0xFF);
  Pot1 = new MCP466_DigitalPot(POT_ADDR1);
  Pot1->setWireInterface(&Wire1);
  Pot1->mcpWrite(TCON, 0xFF);

  M5.Lcd.println("- Init ADC");
  analogReadResolution(ADC_BITS_ESP);

  /*
    M5.Lcd.println("- Init DAC");
    DAC_AD5686.HardReset();
  */

  M5.Lcd.println("- Init GUI");
  lv_init();
  lv_disp_draw_buf_init(&draw_buf, b1, b2, (LCD_WIDTH * LCD_HEIGHT) / 10);
  init_display();
  init_touch();
  // lv_disp_set_theme(NULL, LV_THEME_DEFAULT_DARK);
  //  th_alarm = lv_theme_default_init(NULL, lv_palette_main(LV_PALETTE_RED), lv_palette_main(LV_PALETTE_RED), 1, &lv_font_montserrat_22);
  th = lv_theme_default_init(NULL, lv_palette_main(LV_PALETTE_TEAL), lv_palette_main(LV_PALETTE_RED), LV_THEME_DEFAULT_DARK, &lv_font_montserrat_22);
  lv_disp_set_theme(NULL, th); /*Assign the theme to the display*/
  th_alarm = th;
  th_alarm->color_primary = lv_palette_main(LV_PALETTE_RED);
  // lv_theme_set_parent(th_alarm, th);
  //  lv_disp_set_theme(NULL, th_alarm);

  //  lv_disp_set_bg_color(NULL, lv_palette_main(LV_PALETTE_RED));
  // lv_disp_set_bg_color()

  /*Create a Tab view object*/

  tabview = lv_tabview_create(lv_scr_act(), LV_DIR_LEFT, SCREEN_TAB_WIDTH);
  lv_obj_set_style_text_font(tabview, &lv_font_montserrat_22, LV_PART_MAIN);

  /*Add 3 tabs (the tabs are page (lv_page) and can be scrolled*/
  lv_obj_t *tab1 = lv_tabview_add_tab(tabview, LV_SYMBOL_LIST);
  lv_obj_set_style_text_font(tab1, &lv_font_montserrat_14, LV_PART_MAIN);

  #if CONFIG_MENU_TAB2
  lv_obj_t *tab2 = lv_tabview_add_tab(tabview, LV_SYMBOL_SLIDER);
  lv_obj_set_style_text_font(tab2, &lv_font_montserrat_14, LV_PART_MAIN);
  #endif

  #if CONFIG_MENU_TAB3
  lv_obj_t *tab3 = lv_tabview_add_tab(tabview, LV_SYMBOL_SETTINGS);
  lv_obj_set_style_text_font(tab3, &lv_font_montserrat_14, LV_PART_MAIN);
  #endif

  #if CONFIG_MENU_TAB4
  lv_obj_t *tab4 = lv_tabview_add_tab(tabview, LV_SYMBOL_CHIP);
  lv_obj_set_style_text_font(tab4, &lv_font_montserrat_14, LV_PART_MAIN);
  #endif

  lv_obj_t *tab5 = lv_tabview_add_tab(tabview, LV_SYMBOL_VIDEO);
  lv_obj_set_style_text_font(tab5, &lv_font_montserrat_14, LV_PART_MAIN);

  canvas = lv_canvas_create(tab5);

  #if CONFIG_TWID02_SUPPORT
    camera->init(); // this requires a canvas 
  #elif CONFIG_TWID03_SUPPORT
    // this is done while setting the MEMORY registers for min/max of col/row
  #elif CONFIG_TWID04_SUPPORT
    camera->init(); // this requires a canvas 
  #else
  #error "No valid CONFIG_TWIDxx_SUPPORT defined!"
  #endif

  // Create a performance monitor widget
  perf_monitor_init(&perf_monitor);
  perf_monitor.perf_label = lv_label_create(lv_layer_sys());
  lv_obj_set_style_bg_opa(perf_monitor.perf_label, LV_OPA_50, 0);
  lv_obj_set_style_bg_color(perf_monitor.perf_label, lv_color_black(), 0);
  lv_obj_set_style_text_color(perf_monitor.perf_label, lv_color_white(), 0);
  lv_obj_set_style_pad_top(perf_monitor.perf_label, 3, 0);
  lv_obj_set_style_pad_bottom(perf_monitor.perf_label, 3, 0);
  lv_obj_set_style_pad_left(perf_monitor.perf_label, 3, 0);
  lv_obj_set_style_pad_right(perf_monitor.perf_label, 3, 0);
  lv_obj_set_style_text_align(perf_monitor.perf_label, LV_TEXT_ALIGN_RIGHT, 0);
  lv_label_set_text(perf_monitor.perf_label, "?");
  lv_obj_align(perf_monitor.perf_label, LV_ALIGN_BOTTOM_RIGHT, -SCREEN_TAB_WIDTH, 0);
  lv_obj_add_flag(perf_monitor.perf_label, LV_OBJ_FLAG_HIDDEN);

  int posy = 0;
  #define POS_SKIP_SLIDER 50
  #define POS_SKIP_CHECKBOX 30

  tab_count = lv_obj_get_child_cnt(lv_tabview_get_content(tabview)); // figure out the tab count

  /************ tab1 ************/

  table = lv_table_create(tab1);

  logo = lv_img_create(tab1);
  lv_img_set_src(logo, &TicWaveLogo6_98x30_g);
  lv_obj_align(logo, LV_ALIGN_TOP_LEFT, 15, 5);

  /* Create power off button */
  lv_obj_t *btn_off = lv_btn_create(tab1);
  lv_obj_add_event_cb(btn_off, btn_off_event_handler, LV_EVENT_LONG_PRESSED, NULL);
  // lv_btn_set_checkable(btn_off, false);
  lv_obj_add_flag(btn_off, LV_OBJ_FLAG_CHECKABLE);
  lv_obj_align_to(btn_off, logo, LV_ALIGN_OUT_RIGHT_MID, 60, -10);
  lv_obj_t *btn_off_label = lv_label_create(btn_off);
  lv_label_set_text(btn_off_label, LV_SYMBOL_POWER); 
  lv_obj_set_style_text_font(btn_off_label, &lv_font_montserrat_22, LV_PART_MAIN);
  lv_obj_center(btn_off_label);
//  lv_obj_set_width(btn_off, 80);
//  lv_obj_set_height(btn_off, 50);
  ////lv_btn_set_fit(btn_off, LV_FIT_TIGHT);

  lv_table_set_col_cnt(table, 2);
  lv_table_set_col_width(table, 0, 115);
  lv_table_set_col_width(table, 1, 125);
  lv_obj_align(table, LV_ALIGN_TOP_MID, 0, 50);

  /* Fill the first column names */
  int line = 0;

  lv_table_set_cell_value(table, line, 0, "Firmware\nRevision");
  lv_table_set_cell_value_fmt(table, line++, 1, "%s\n%s", ThisDevice->firmware, ThisDevice->revision);

  #if CONFIG_WIFI_AP  
    lv_table_set_cell_value(table, line, 0, "WiFi SSID"); 
    lv_table_set_cell_value_fmt(table, line++, 1, "%s", ThisDevice->wifi_ssid);  
    lv_table_set_cell_value(table, line, 0, "WiFi PASSWD"); 
    lv_table_set_cell_value_fmt(table, line++, 1, "%s", ThisDevice->wifi_passwd);  
//    lv_table_set_cell_value(table, line, 0, "IP"); 
//    lv_table_set_cell_value_fmt(table, line++, 1, "%s", ThisDevice->wifi_ip);  
    MYM5_WIFI = new MyM5(MYM5_IDX_WIFI, "WIFI Status", "%s");
    MYM5_WIFI->TableCreate(line++, table);

  #endif

  // read Chip ID
  #if CONFIG_MONITOR_VERBOSE
    MYM5_DATE = new MyM5(MYM5_IDX_DATE, "DATE", "%s");
    MYM5_DATE->TableCreate(line++, table);

  camera->ReadMem(ASIC_CAM_CHIP_ID);
  lv_table_set_cell_value(table, line, 0, "Chip ID"); 
  lv_table_set_cell_value_fmt(table, line++, 1, "%d", camera->rx_data);  
  #endif
  // Diode based temp sensor of SARRAY
 
  #if CONFIG_MONITOR_VERBOSE
  TEMP_CAMERA = new MyTEMP("Chip temp", "%.2f °C", 3.3 / ADC_RANGE_ADS1100);
  TEMP_CAMERA->TableCreate(line, table);
  lv_table_set_cell_value_fmt(table, line++, 1, "%s", "--"); 

  ADC_TEMP = new MyADCi2c(ADS1100_AD6_ADDRESS, "ADC temp", "%.4f V", ADC_RANGE_ADS1100-1, 3.3 / ADC_RANGE_ADS1100);
  ADC_TEMP->TableCreate(line, table);
  lv_table_set_cell_value_fmt(table, line++, 1, "%s", "--"); 

  MYM5_TEMP_APX192 = new MyM5(MYM5_IDX_TEMP_APX192, "APX temp", "%2.1f °C");
  MYM5_TEMP_APX192->TableCreate(line++, table);
  #endif
  
  // status on M5 internals
  MYM5_AC = new MyM5(MYM5_IDX_AC, "AC Supply", "%s %2.1f V\n%6.1f mA");
  MYM5_AC->TableCreate(line++, table);
  
  MYM5_CHARGE = new MyM5(MYM5_IDX_CHARGE, "Bat Charge\nDischarge", "%s %6.1f mA\n%6.1f mW");
  MYM5_CHARGE->TableCreate(line++, table);
  
  MYM5_BAT = new MyM5(MYM5_IDX_BAT, "Bat Status", "%s %2.1f V\n%2.1f %%");
  MYM5_BAT->TableCreate(line++, table);

  #if CONFIG_MONITOR_VERBOSE
  MYM5_VBUS = new MyM5(MYM5_IDX_VBUS, "Supply\nTo LDO", "%s %2.1f V\n%6.1f mA");
  MYM5_VBUS->TableCreate(line++, table);

  MYM5_IPS = new MyM5(MYM5_IDX_IPS, "IPS Supply", "%2.1f V");
  MYM5_IPS->TableCreate(line++, table);
  #endif

  /************ tab2 ************/
  #if CONFIG_MENU_TAB2
  lv_obj_set_style_pad_bottom(tab2, 50, LV_PART_MAIN);

  // DAC1-4 sliders
  char buf[40];
  posy = 0;
  // source sliders
  FEATURE_ENABLE_SLIDER = new MyFeature(FEATURE_IDX_ENABLE_SLIDER, "Enable bias slider", 0);
  FEATURE_ENABLE_SLIDER->CheckboxCreate(posy += POS_SKIP_CHECKBOX, tab2);

  POT_REF = new MyPOT(Pot0, 1, "Vref", "%.2f V", 40, 0, 67, 5.0 / POT_RANGE); // default: 0.77=40, max 1.3V=67
  POT_REF->SliderCreate(posy += POS_SKIP_SLIDER, LV_SLIDER_MODE_NORMAL, tab2);
  POT_REF->TableCreate(line++, table);

  POT_DET = new MyPOT(Pot0, 0, "VCC Det", "%.2f V", 60, 0, 67, 5.0 / POT_RANGE); // default: 1.2V = 60, max 1.3V=67
  POT_DET->SliderCreate(posy += POS_SKIP_SLIDER, LV_SLIDER_MODE_NORMAL, tab2);
  POT_DET->TableCreate(line++, table);

  POT_VCO = new MyPOT(Pot1, 0, "VDD VCO", "%.2f V", 67, 0, 70, 5.0 / POT_RANGE); // default: 1.2V = 60, max 1.3V=67
  POT_VCO->SliderCreate(posy += POS_SKIP_SLIDER, LV_SLIDER_MODE_NORMAL, tab2);
  POT_VCO->TableCreate(line++, table);

  FEATURE_ENABLE_SLIDER->DoIt(); // this feature needs to be called after sliders are created

#if (CONFIG_BIAS_SLIDERS == 0)
 FEATURE_ENABLE_SLIDER->Hide();
 POT_REF->HideSlider();
 POT_DET->HideSlider();
 POT_VCO->HideSlider();
 posy -= 4 * POS_SKIP_SLIDER;
//  Pot0->mcpWrite(1, 255 - 40);
//  Pot0->mcpWrite(0, 255 - 60);
//  Pot1->mcpWrite(0, 255 - 67);
#endif

    #if CONFIG_TWID02_SUPPORT
  MEM_PCLK_LB = new MyMEM(ASIC_CAM_PCLK_CYC_LBYTE, "Integration", "  %3.0f", DEFAULT_INTEGRATION, 20, 255, 1.0); // default 0x90
    #elif CONFIG_TWID03_SUPPORT
  MEM_PCLK_LB = new MyMEM(ASIC_CAM_PCLK_CYC_LBYTE, "Integration", "  %3.0f", 50, 20, 255, 1.0); // default 100 for id02 and 50 for id03
    #elif CONFIG_TWID04_SUPPORT
    #else
  MEM_PCLK_LB = new MyMEM(ASIC_CAM_PCLK_CYC_LBYTE, "Integration", "  %3.0f", 40, 20, 255, 1.0); // default 40 for ID03 and ID04
    #endif
  MEM_PCLK_LB->SliderCreate(posy += POS_SKIP_SLIDER, LV_SLIDER_MODE_NORMAL, tab2);
  MEM_PCLK_LB->TableCreate(line++, table);
  MEM_PCLK_LB->DoIt();

    #if CONFIG_TWID02_SUPPORT
  MEM_PCLK_HB = new MyMEM(ASIC_CAM_PCLK_CYC_HBYTE, "Integration (HB)", " %3.0f", 0x00, 0, 0b00000111, 1.0); // default 0x01
    #elif CONFIG_TWID03_SUPPORT
  MEM_PCLK_HB = new MyMEM(ASIC_CAM_PCLK_CYC_HBYTE, "Integration (HB)", " %3.0f", 0, 0, 0b00000111, 1.0); // default 0 for id03 and 1 for id02
    #elif CONFIG_TWID04_SUPPORT
    #else
    #endif

  MEM_PCLK_HB->SliderCreate(posy += POS_SKIP_SLIDER, LV_SLIDER_MODE_NORMAL, tab2);
  MEM_PCLK_HB->TableCreate(line++, table);
  MEM_PCLK_HB->DoIt();

  CAMERA_XCLK = new MyXCLK(CAM_PIN_XCLK, (uint8_t)LEDC_CHANNEL_0, "Chip XCLK", "%2.0f MHz", 15, 1, 40, 1.0); // not 80 due to dutycycle
  CAMERA_XCLK->SliderCreate(posy += POS_SKIP_SLIDER, LV_SLIDER_MODE_NORMAL, tab2);
  CAMERA_XCLK->TableCreate(line++, table);
  CAMERA_XCLK->DoIt();

  MEM_HSYNC = new MyMEM(ASIC_CAM_HSYNC_CYC, "HSYNC Cyc.", " %3.0f", 0x04, 0, 0xF, 1.0);
  MEM_HSYNC->SliderCreate(posy += POS_SKIP_SLIDER, LV_SLIDER_MODE_NORMAL, tab2);
  MEM_HSYNC->TableCreate(line++, table);
  MEM_HSYNC->DoIt();

#if (CONFIG_EXTENDED_CLK_SETINGS == 0) // hide the previous 3 slider
  CAMERA_XCLK->HideSlider();
  MEM_PCLK_LB->HideSlider();
  MEM_PCLK_HB->HideSlider();
  MEM_HSYNC->HideSlider();
  posy -= 4 * POS_SKIP_SLIDER;
//  posy -= 80;
//  camera->WriteMem(ASIC_CAM_PCLK_CYC_HBYTE, (uint8_t) 0x00);
//  camera->WriteMem(ASIC_CAM_PCLK_CYC_HBYTE, (uint8_t) 0x04);
#endif

  /* Create average roller */
  roller_avr = lv_roller_create(tab2);
  lv_roller_set_options(roller_avr,
                        "1\n"
                        "4\n"
                        "8\n"
                        "16",
                        LV_ROLLER_MODE_INFINITE);
  lv_roller_set_visible_row_count(roller_avr, 2);
  lv_obj_set_pos(roller_avr, 10, posy += 70);
  lv_obj_add_event_cb(roller_avr, roller_avr_event_handler, LV_EVENT_ALL, NULL);
  lv_roller_set_selected(roller_avr, 0, LV_ANIM_OFF); // select
  lv_obj_t *roller_avr_label = lv_label_create(tab2);
  lv_label_set_text(roller_avr_label, "Average");
  lv_obj_align_to(roller_avr_label, roller_avr, LV_ALIGN_OUT_TOP_MID, 0, -10);

  /* Create colormap roller */
  roller_colormap = lv_roller_create(tab2);
  lv_roller_set_options(roller_colormap,
                        "GRAY\n"
                        "JET\n"
                        "WINTER\n"
                        "COOL\n"
                        "AUTUMN\n"
                        "SPRING",
                        LV_ROLLER_MODE_INFINITE);
  lv_roller_set_visible_row_count(roller_colormap, 2);
  //    lv_obj_set_pos(roller_colormap, 150, posy);
  lv_obj_set_pos(roller_colormap, 70, posy);
  //    lv_obj_align(roller_colormap, settings_container, LV_ALIGN_IN_LEFT_MID, 0, 0);
  lv_obj_add_event_cb(roller_colormap, roller_colormap_event_handler, LV_EVENT_ALL, NULL);
  lv_roller_set_selected(roller_colormap, 0, LV_ANIM_OFF); // select
  /* Create an informative label above the slider */
  lv_obj_t *roller_colormap_label = lv_label_create(tab2);
  lv_label_set_text(roller_colormap_label, "Color Map");
  lv_obj_align_to(roller_colormap_label, roller_colormap, LV_ALIGN_OUT_TOP_MID, 0, -10);
  roller_chopper_cyc = lv_roller_create(tab2);
    #if CONFIG_TWID02_SUPPORT
  /* Create chopper cyc roller */
  lv_roller_set_options(roller_chopper_cyc,
                        "1\n"
                        "2\n"
                        "3\n"
                        "4\n"
                        "5\n"
                        "6\n"
                        "7\n"
                        "9\n"
                        "12\n"
                        "18\n"
                        "35",
                        LV_ROLLER_MODE_INFINITE);
    #elif CONFIG_TWID03_SUPPORT
  /* Create chopper cyc roller */
  lv_roller_set_options(roller_chopper_cyc,
                        "1\n"
                        "2\n"
                        "3\n"
                        "4\n"
                        "5\n"
                        "6\n"
                        "8\n"
                        "9\n"
                        "11\n"
                        "14\n"
                        "17\n"
                        "22\n"
                        "33\n"
                        "66",
                        LV_ROLLER_MODE_INFINITE);
    #elif CONFIG_TWID04_SUPPORT
  lv_roller_set_options(roller_chopper_cyc,
                        "1\n"
                        "2\n"
                        "3\n"
                        "4\n"
                        "5\n"
                        "6\n"
                        "8\n"
                        "9\n"
                        "12\n"
                        "18\n"
                        "36",
                        LV_ROLLER_MODE_INFINITE);
    #else
    #endif

  lv_roller_set_visible_row_count(roller_chopper_cyc, 2);
  lv_obj_set_pos(roller_chopper_cyc, 180, posy);
  lv_obj_add_event_cb(roller_chopper_cyc, roller_chopper_cyc_handler, LV_EVENT_ALL, NULL);
  lv_roller_set_selected(roller_chopper_cyc, 1, LV_ANIM_OFF); // select
  lv_event_send(roller_chopper_cyc, LV_EVENT_VALUE_CHANGED, NULL);
  /* Create an informative label above the slider */
  lv_obj_t *roller_chopper_cyc_label = lv_label_create(tab2);
  lv_label_set_text(roller_chopper_cyc_label, "Chop. Cyc.");
  lv_obj_align_to(roller_chopper_cyc_label, roller_chopper_cyc, LV_ALIGN_OUT_TOP_MID, 0, -10);

  CAMERA_EXPOSER = new MyEXPOSER("Expo. Time", " %3.0f µs", DEFAULT_INTEGRATION, 45, 4500, 1.0);  // µ need to be addd in the ASCII default range, e.g. default=['0x20-0x7F,0xB0,0x2022,0xB5'] in: components/lvgl/scripts/built_in_font/built_in_font_gen.py, then re run generate_all.py
  CAMERA_EXPOSER->SliderCreate(posy += 2*POS_SKIP_SLIDER +20, LV_SLIDER_MODE_NORMAL, tab2);
  CAMERA_EXPOSER->TableCreate(line++, table);
  CAMERA_EXPOSER->DoIt();

 #if CONFIG_TWID03_SUPPORT
  MEM_ROW_MIN_MAX = new MyMEM(ASIC_CAM_ROW_MAX, "ROW zoom", " %2.0f-%2.0f", 47, 0, 47, 1.0);
  MEM_ROW_MIN_MAX->SliderCreate(posy += POS_SKIP_SLIDER, LV_SLIDER_MODE_RANGE, tab2);
  MEM_ROW_MIN_MAX->TableCreate(line++, table);
  MEM_ROW_MIN_MAX->DoIt(); // do this later

  MEM_COL_MIN_MAX = new MyMEM(ASIC_CAM_COL_MAX, "COL zoom", " %2.0f-%2.0f", 63, 0, 63, 1.0);
  MEM_COL_MIN_MAX->SliderCreate(posy += POS_SKIP_SLIDER, LV_SLIDER_MODE_RANGE, tab2);
  MEM_COL_MIN_MAX->TableCreate(line++, table);
  MEM_COL_MIN_MAX->DoIt(); // do this later
#endif

  /*Create a slider in the center of the display*/
  slider_color_scale_max = lv_slider_create(tab2);
  // lv_obj_center(slider_color_scale_max);
  lv_obj_set_width(slider_color_scale_max, 130 * 1.2);
  lv_obj_set_pos(slider_color_scale_max, 30, posy += 2*POS_SKIP_SLIDER -20);

  lv_slider_set_range(slider_color_scale_max, 0, RANGE_TWID02-1); // ADC_RANGE_AD7924, we can get up to the full uint16_t bit resolution due to averaging!
  lv_slider_set_mode(slider_color_scale_max, LV_SLIDER_MODE_RANGE);
  lv_slider_set_value(slider_color_scale_max, 100, LV_ANIM_OFF);
  lv_slider_set_left_value(slider_color_scale_max, 0, LV_ANIM_OFF);
  lv_obj_t *slider_colorscale_label = lv_label_create(tab2);
  lv_label_set_text(slider_colorscale_label, "Color Scale");
  lv_obj_align_to(slider_colorscale_label, slider_color_scale_max, LV_ALIGN_OUT_TOP_MID, 0, -30);
  lv_obj_add_event_cb(slider_color_scale_max, slider_color_scale_max_event_cb, LV_EVENT_ALL, NULL);
  lv_obj_refresh_ext_draw_size(slider_color_scale_max);

  #endif

  /************ tab3 ************/
  #if CONFIG_MENU_TAB3

//  lv_obj_set_scrollbar_mode(tab3, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_style_pad_all(tab3, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(tab3, 20, LV_PART_MAIN);
  posy = 0;

#if CONFIG_WIFI_AP
  FEATURE_WIFI = new MyFeature(FEATURE_IDX_WIFI, "Enable WIFI AP", 0);
  FEATURE_WIFI->CheckboxCreate(posy+=POS_SKIP_CHECKBOX,tab3);
  FEATURE_WIFI->DoIt(); 
#endif

  FEATURE_SUBST = new MyFeature(FEATURE_IDX_SUBST, "EN Chopped/CW", 1);
  FEATURE_SUBST->CheckboxCreate(posy += POS_SKIP_CHECKBOX, tab3);
  FEATURE_SUBST->DoIt();

  FEATURE_AUTOSCALE = new MyFeature(FEATURE_IDX_AUTOSCALE, "Autoscale video", 1);
  FEATURE_AUTOSCALE->CheckboxCreate(posy += POS_SKIP_CHECKBOX, tab3);
  FEATURE_AUTOSCALE->DoIt();

  FEATURE_PERSISTANT = new MyFeature(FEATURE_IDX_PERSISTANT, "Infinit Persistance", 0);
  FEATURE_PERSISTANT->CheckboxCreate(posy += POS_SKIP_CHECKBOX, tab3);
  FEATURE_PERSISTANT->DoIt();

  FEATURE_HMIRROR = new MyFeature(FEATURE_IDX_HMIRROR, "H-Mirror", 0);
  FEATURE_HMIRROR->CheckboxCreate(posy += POS_SKIP_CHECKBOX, tab3);
  FEATURE_HMIRROR->DoIt();

  FEATURE_VMIRROR = new MyFeature(FEATURE_IDX_VMIRROR, "V-Mirror", 0);
  FEATURE_VMIRROR->CheckboxCreate(posy += POS_SKIP_CHECKBOX, tab3);
  FEATURE_VMIRROR->DoIt();

  FEATURE_ROTATE90 = new MyFeature(FEATURE_IDX_ROTATE90, "Rotate +90", 0);
  FEATURE_ROTATE90->CheckboxCreate(posy += POS_SKIP_CHECKBOX, tab3);
  FEATURE_ROTATE90->DoIt();

  FEATURE_WRITE_INT16 = new MyFeature(FEATURE_IDX_WRITE_INT16, "int16 data format", 0);
  FEATURE_WRITE_INT16->CheckboxCreate(posy += POS_SKIP_CHECKBOX, tab3);
  FEATURE_WRITE_INT16->DoIt();

#if CONFIG_VIDEO_FILTER
  FEATURE_FILT = new MyFeature(FEATURE_IDX_FILT, "Enable video filter", 0);
  FEATURE_FILT->CheckboxCreate(posy += POS_SKIP_CHECKBOX, tab3);
  FEATURE_FILT->DoIt();
#endif

#if CONFIG_BILINEAR_INTERPOLATION
  FEATURE_INTERPOL = new MyFeature(FEATURE_IDX_INTERPOL, "Sub-Pixel Interpolation", 0);
  FEATURE_INTERPOL->CheckboxCreate(posy += POS_SKIP_CHECKBOX, tab3);
  FEATURE_INTERPOL->DoIt();
#endif 


  FEATURE_ZOOM = new MyFeature(FEATURE_IDX_ZOOM, "Image Zoom", 1);
  FEATURE_ZOOM->CheckboxCreate(posy += POS_SKIP_CHECKBOX, tab3);
  FEATURE_ZOOM->DoIt();
#if (CONFIG_IMAGE_ZOOM==0)
  FEATURE_ZOOM->Hide();
  posy -= POS_SKIP_CHECKBOX;
#endif 

  FEATURE_MONITOR = new MyFeature(FEATURE_IDX_MONITOR, "Performance Monitor", 0); // switch on for now
  FEATURE_MONITOR->CheckboxCreate(posy += POS_SKIP_CHECKBOX, tab3);
  FEATURE_MONITOR->DoIt();

  FEATURE_VC = new MyFeature(FEATURE_IDX_VC, "EN Video Controller", 1);
  FEATURE_VC->CheckboxCreate(posy += POS_SKIP_CHECKBOX, tab3);
  FEATURE_VC->DoIt();
  
  FEATURE_VPORCH = new MyFeature(FEATURE_IDX_VPORCH, "Add VPORCH", 1);
  FEATURE_VPORCH->CheckboxCreate(posy += POS_SKIP_CHECKBOX, tab3);
  FEATURE_VPORCH->DoIt();

  // the chopper_out_pad to source = (chopper_select[1] == 1) ? chopper_in_pad : chopper_int;
  // the used chopper of camera array = (chopper_select[0] == 1) ?  chopper_in_pad : chopper_int;
  FEATURE_CHOPPER_SYNC = new MyFeature(FEATURE_IDX_CHOPPER_SYNC, "Chopper Master", 1); // if deselected MCU needs to provide a chopper signal
  FEATURE_CHOPPER_SYNC->CheckboxCreate(posy += POS_SKIP_CHECKBOX, tab3);
  FEATURE_CHOPPER_SYNC->DoIt();

#if (CONFIG_VIDEOCONTROLLER == 0)
  FEATURE_VC->Hide();
  FEATURE_VPORCH->Hide();
  FEATURE_CHOPPER_SYNC->Hide();
#endif
  
  #endif

  /************ tab4 ************/
  #if CONFIG_MENU_TAB4
  
  posy = 0;

  static const char *map_reg_bits[] = {"7", "6", "5", "4", "3", "2", "1", "0", "\n",
                                       "7", "6", "5", "4", "3", "2", "1", "0", "\n",
                                       "Transfer", ""};
  btnm4 = lv_btnmatrix_create(tab4); // pixel btnm
  lv_obj_set_style_text_font(btnm4, &lv_font_montserrat_22, LV_PART_MAIN);

  static lv_style_t style_bg;
  lv_style_init(&style_bg);
  lv_style_set_pad_all(&style_bg, 0);
  lv_style_set_pad_gap(&style_bg, 0);
  lv_style_set_clip_corner(&style_bg, false);
  lv_style_set_radius(&style_bg, 0);
  lv_style_set_border_width(&style_bg, 1);
  lv_style_set_border_side(&style_bg, LV_BORDER_SIDE_FULL);
  lv_style_set_shadow_width(&style_bg, 0);
  lv_style_set_outline_width(&style_bg, 0);

  static lv_style_t style_btn;
  lv_style_init(&style_btn);
  lv_style_set_radius(&style_btn, 0);
  lv_style_set_border_width(&style_btn, 1);
  lv_style_set_border_opa(&style_btn, LV_OPA_50);
  lv_style_set_border_color(&style_btn, lv_palette_main(LV_PALETTE_GREY));
  lv_style_set_border_side(&style_btn, LV_BORDER_SIDE_FULL);
  lv_style_set_shadow_width(&style_btn, 0);
  lv_style_set_radius(&style_btn, 0);
  lv_style_set_outline_width(&style_btn, 0);

  lv_obj_add_style(btnm4, &style_bg, 0);
  lv_obj_add_style(btnm4, &style_btn, LV_PART_ITEMS);

  lv_obj_set_size(btnm4, CANVAS_WIDTH, 110);
  lv_obj_set_pos(btnm4, 0, posy += 15);
  lv_obj_add_event_cb(btnm4, btnm4_event_handler, LV_EVENT_ALL, NULL);

  //  lv_style_set_bg_color(&btnStyle, lv_palette_main(LV_PALETTE_ORANGE));
  //  lv_obj_add_style(btnm4, &btnStyle , LV_STATE_CHECKED);

  //  lv_obj_set_style_bg_color(btnm4,lv_color_white(),LV_PART_ITEMS);
  //  lv_obj_set_style_bg_color(btnm4,lv_palette_main(LV_PALETTE_ORANGE),LV_STATE_CHECKED);
  //  lv_palette_main(LV_PALETTE_ORANGE)

  lv_obj_set_style_bg_color(btnm4, lv_color_white(), LV_PART_ITEMS);
  lv_btnmatrix_set_map(btnm4, map_reg_bits);
  //    lv_btnmatrix_set_btn_width(btnm4, 10, 2);
  for (uint8_t i = 0; i < 8; i++)
    lv_btnmatrix_set_btn_ctrl(btnm4, i, LV_BTNMATRIX_CTRL_CHECKABLE);
  for (uint8_t i = 8; i < 16; i++)
    lv_btnmatrix_set_btn_ctrl(btnm4, i, LV_BTNMATRIX_CTRL_DISABLED);
  lv_btnmatrix_clear_btn_ctrl(btnm4, 16, LV_BTNMATRIX_CTRL_CHECKABLE);

  lv_obj_t *roller_RW = lv_roller_create(tab4);
  lv_roller_set_options(roller_RW,
                        "R\nW",
                        LV_ROLLER_MODE_INFINITE);
  //    lv_obj_set_style_text_font(roller_RW, &lv_font_montserrat_14, LV_PART_MAIN);

  lv_obj_add_event_cb(roller_RW, roller_RW_event_handler, LV_EVENT_ALL, NULL);
  lv_roller_set_visible_row_count(roller_RW, 1);
  lv_obj_set_pos(roller_RW, 0, posy += 120);
  lv_obj_set_size(roller_RW, 30, 30); //(lv_coord_t) CANVAS_WIDTH/8 +1
                                      //    lv_obj_add_style(roller_RW, &style_btn, LV_PART_ITEMS);
  lv_obj_add_style(roller_RW, &style_bg, 0);

  lv_obj_t *roller_CMD = lv_roller_create(tab4);
  lv_roller_set_options(roller_CMD,
                        "CHIP_ID\nCHOP_DIV\nPCLK_CYC_LBYTE\nPCLK_CYC_HBYTE\nHSYNC_CYC\nHPORCH\nVPORCH\nVC_EN\nSUBTRACT_EN\nCHOPPER_SELECT\nLNA_LB\nLNA_HB\nOSC_LB\nOSC_HB\nDOUB_LB\nDOUB_HB\nDOUB_LB\nDOUB_MB\nDOUB_HB",
                  LV_ROLLER_MODE_INFINITE);

  lv_obj_add_event_cb(roller_CMD, roller_CMD_event_handler, LV_EVENT_ALL, NULL);
  lv_roller_set_visible_row_count(roller_CMD, 1);
  lv_obj_set_pos(roller_CMD, 30, posy);
  lv_obj_set_size(roller_CMD, (lv_coord_t)7 * CANVAS_WIDTH / 8, 30);
  lv_obj_add_style(roller_CMD, &style_bg, 0);
  /*
      lv_obj_t * roller_ADDR_DATA = lv_roller_create(tab4);
      lv_roller_set_options(roller_ADDR_DATA,
                            "0-0000\n1-0001\n2-0010\n3-0011\n4-0100\n5-0101\n6-0110\n7-0111\n8-1000\n9-1001\n10-1010\n11-1011\n12-1100\n13-1101\n14-1110\n15-1111",
                            LV_ROLLER_MODE_INFINITE);
      lv_roller_set_visible_row_count(roller_ADDR_DATA, 1);
      lv_obj_set_pos(roller_ADDR_DATA, 120, posy);
      lv_obj_add_event_cb(roller_ADDR_DATA, roller_ADDR_DATA_event_handler, LV_EVENT_ALL, NULL);
      lv_obj_set_size(roller_ADDR_DATA, (lv_coord_t) 4*CANVAS_WIDTH/8, 30);
      lv_obj_add_style(roller_ADDR_DATA, &style_bg, 0);
  */

  #endif
  /************ tab5 ************/

  lv_obj_set_scrollbar_mode(tab5, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_style_pad_all(tab5, 0, LV_PART_MAIN);

  /*
     img = lv_img_create(tab5);
     lv_img_set_src(img, &video_img);
     lv_img_set_antialias(img, false);
     lv_img_set_size_mode(img, LV_IMG_SIZE_MODE_REAL);
  */

  //    lv_img_set_auto_size(img, true);
  //    lv_obj_set_width(img, LV_HOR_RES);
  //    lv_obj_set_width(img, 240);
  //    lv_obj_set_height(img, 240);
  ////   lv_obj_set_style_pad_all(img, 0, 0);
  ////   lv_img_set_pivot(img, 0, 0);
  //    lv_img_set_zoom(img, 25*256); // 30x zoom
  ////    lv_img_set_zoom(img, 2*2560);
  // lv_obj_set_pos(img, 320/2 -56, 240/2 -16);
  ////    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
  //    lv_img_set_offset_x(img, 15);
  //    lv_img_set_offset_y(img, 15);
  //    lv_obj_set_pos(img, 0, 0);

  //init_canvas();
  //MEM_COL_MIN_MAX->DoIt(); // do this later
  //MEM_ROW_MIN_MAX->DoIt(); // do this later
//  canvas = lv_canvas_create(tab5);
//  lv_canvas_set_buffer(canvas, cbuf, CANVAS_WIDTH, CANVAS_HEIGHT, LV_IMG_CF_TRUE_COLOR);
//  lv_obj_set_pos(canvas, int(240 - CANVAS_WIDTH) / 2, int(240 - CANVAS_WIDTH) / 2); // try to center
//  lv_canvas_fill_bg(canvas, lv_palette_lighten(LV_PALETTE_GREY, 3), LV_OPA_COVER);
  lv_obj_add_event_cb(canvas, canvas_event_handler, LV_EVENT_ALL, NULL);
  lv_obj_add_flag(canvas, LV_OBJ_FLAG_CLICKABLE);

  btnm3 = lv_btnmatrix_create(tab5); // ctrl btnm
  lv_obj_set_style_text_font(btnm3, &lv_font_montserrat_22, LV_PART_MAIN);
  lv_btnmatrix_set_map(btnm3, btnm_map_ctrl2);

  //    lv_btnmatrix_set_btn_ctrl(btnm3, 0, LV_BTNMATRIX_CTRL_CHECKABLE);
  //    lv_btnmatrix_set_btn_ctrl(btnm3, 1, LV_BTNMATRIX_CTRL_CHECKABLE);
  //    lv_btnmatrix_set_btn_ctrl(btnm3, 4, LV_BTNMATRIX_CTRL_CHECKABLE);
  lv_btnmatrix_set_btn_ctrl_all(btnm3, LV_BTNMATRIX_CTRL_CHECKABLE);
  if(en_zero) lv_btnmatrix_set_btn_ctrl(btnm3, 1, LV_BTNMATRIX_CTRL_CHECKED);
  // lv_btnmatrix_set_one_checked(btnm3,true);

  lv_obj_set_width(btnm3, SCREEN_TAB_WIDTH);
  lv_obj_set_height(btnm3, 240);
  lv_obj_set_pos(btnm3, 240, 0);

  lv_obj_set_style_pad_all(btnm3, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(btnm3, 0, LV_PART_ITEMS);

  lv_obj_set_style_pad_gap(btnm3, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_gap(btnm3, 0, LV_PART_ITEMS);

  lv_obj_set_style_border_width(btnm3, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(btnm3, 0, LV_PART_ITEMS);

  lv_obj_set_style_radius(btnm3, 0, LV_PART_ITEMS);

  lv_obj_add_event_cb(btnm3, btnm3_event_handler, LV_EVENT_ALL, NULL);

  /*    if(FEATURE_MONITOR->state) lv_btnmatrix_set_btn_ctrl(btnm3, 5, LV_BTNMATRIX_CTRL_CHECKED);
       SCOPE = new MyScope(SCOPE_TYPE_VIDEO, "Scope", canvas, ADC_BIST);
  */

// Chart
  chart = lv_chart_create(tab5);
  lv_obj_add_flag(chart, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_size(chart, 170, 200);     
  lv_obj_align(chart, LV_ALIGN_CENTER, 10, -10);
  lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_Y, 10, 5, 6, 5, true, 60);
  lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_X, 10, 5, 10, 1, true, 30);
  lv_obj_add_event_cb(chart, chart_event_cb, LV_EVENT_ALL, NULL);
  lv_obj_refresh_ext_draw_size(chart);
  lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, chart_min, chart_max); //RANGE_TWID02
  cursor = lv_chart_add_cursor(chart, lv_palette_main(LV_PALETTE_BLUE), LV_DIR_LEFT | LV_DIR_BOTTOM);
  ser = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
  //lv_chart_set_zoom_x(chart, 500);
//  lv_obj_t * label = lv_label_create(lv_scr_act());
//  lv_label_set_text(label, "Time Series");
//  lv_obj_align_to(label, chart, LV_ALIGN_OUT_TOP_MID, 0, -5);
  
// histogram

/*Create a histo*/
    histo = lv_chart_create(tab5);
    lv_obj_add_flag(histo, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_size(histo, 190, 200);
    lv_obj_align(histo, LV_ALIGN_CENTER, 0, -10);
//    lv_obj_center(histo);
    lv_chart_set_type(histo, LV_CHART_TYPE_BAR);
    lv_chart_set_range(histo, LV_CHART_AXIS_PRIMARY_Y, 0, histo_max);
//    lv_chart_set_range(histo, LV_CHART_AXIS_PRIMARY_X, -6, 6);
    lv_chart_set_point_count(histo, 13);
//    lv_obj_add_event_cb(histo, histo_event_cb, LV_EVENT_DRAW_PART_BEGIN, NULL);
    /*Add ticks and label to every axis*/
    lv_chart_set_axis_tick(histo, LV_CHART_AXIS_PRIMARY_X, 10, 0, 13, 1, true, 20);
    lv_chart_set_axis_tick(histo, LV_CHART_AXIS_PRIMARY_Y, 10, 5, 6, 2, true, 30);
    lv_obj_set_style_pad_column(histo, 0, LV_PART_ITEMS);   /*Space between columns of the same index*/
    lv_obj_set_style_pad_column(histo, 0, LV_PART_MAIN);    /*Space between columns of the adjacent index*/
 //   lv_chart_set_zoom_x(histo, 800);
    histo_ser = lv_chart_add_series(histo, TicWaveGreen, LV_CHART_AXIS_PRIMARY_Y);
    lv_coord_t * histo_array = lv_chart_get_y_array(histo, histo_ser);
    for(uint8_t i = 0; i < 13; i++) {
//        lv_chart_set_next_value(histo, histo_ser, 0);
        histo_array[i] = 0;
    }

  SCOPE = new MyScope(SCOPE_TYPE_VIDEO, "Scope", canvas, NULL);
  if(SCOPE->LoadFrame(SCOPE->offset) == ESP_OK) en_zero = true;
  lv_btnmatrix_set_btn_ctrl(btnm3, 1, LV_BTNMATRIX_CTRL_CHECKED);

  timer = lv_timer_create(my_timer_cb, TIMER_PERIOD_MS, NULL); // update lable every 30 seconds

  en_lvgl = true;
  // setup done

  CmdReceived = new MyScan(" ");

  // camera->initVSync(CAM_PIN_VSYNC); // for now..
  M5.Lcd.println("- Boot DONE!");
  M5.Axp.SetLed(0);
  sleep(1);
}

/********************/
/* call back funct. */
/********************/
/*
static void histo_event_cb(lv_event_t * e)
{
    lv_obj_draw_part_dsc_t * dsc = lv_event_get_draw_part_dsc(e);
    if(!lv_obj_draw_part_check_type(dsc, &lv_chart_class, LV_CHART_DRAW_PART_TICK_LABEL)) return;

    if(dsc->id == LV_CHART_AXIS_PRIMARY_X && dsc->text) {
        const char * month[] = {"-6", "-5", "-4", "-3", "-2", "-1", "0", "+1", "+2", "+3", "+4", "+5", "+6"};
        lv_snprintf(dsc->text, dsc->text_length, "%s", month[dsc->value]);
    }
}
*/

static void chart_event_cb(lv_event_t * e)
{
    static int32_t last_id = -1;
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_VALUE_CHANGED) {
        last_id = lv_chart_get_pressed_point(obj);
        if(last_id != LV_CHART_POINT_NONE) {
            lv_chart_set_cursor_point(obj, cursor, NULL, last_id);
        }
    }
    else if(code == LV_EVENT_DRAW_PART_END) {
        lv_obj_draw_part_dsc_t * dsc = lv_event_get_draw_part_dsc(e);
        if(!lv_obj_draw_part_check_type(dsc, &lv_chart_class, LV_CHART_DRAW_PART_CURSOR)) return;
        if(dsc->p1 == NULL || dsc->p2 == NULL || dsc->p1->y != dsc->p2->y || last_id < 0) return;

        lv_coord_t * data_array = lv_chart_get_y_array(chart, ser);
        lv_coord_t v = data_array[last_id];
        char buf[16];
        lv_snprintf(buf, sizeof(buf), "%d", v);

        lv_point_t size;
        lv_txt_get_size(&size, buf, LV_FONT_DEFAULT, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);

        lv_area_t a;
        a.y2 = dsc->p1->y - 5;
        a.y1 = a.y2 - size.y - 10;
        a.x1 = dsc->p1->x + 10;
        a.x2 = a.x1 + size.x + 10;

        lv_draw_rect_dsc_t draw_rect_dsc;
        lv_draw_rect_dsc_init(&draw_rect_dsc);
        draw_rect_dsc.bg_color = lv_palette_main(LV_PALETTE_BLUE);
        draw_rect_dsc.radius = 3;

        lv_draw_rect(dsc->draw_ctx, &draw_rect_dsc, &a);

        lv_draw_label_dsc_t draw_label_dsc;
        lv_draw_label_dsc_init(&draw_label_dsc);
        draw_label_dsc.color = lv_color_white();
        a.x1 += 5;
        a.x2 -= 5;
        a.y1 += 5;
        a.y2 -= 5;
        lv_draw_label(dsc->draw_ctx, &draw_label_dsc, &a, buf, NULL);
    }
}

static void canvas_event_handler(lv_event_t *e)
{
  //   lv_obj_t * obj = lv_event_get_target(e);
  if (e->code == LV_EVENT_CLICKED)
  {
    lv_indev_t *indev = lv_indev_get_act();
    lv_point_t p;
    lv_indev_get_point(indev, &p);
  //  cursor_col = (byte)(p.x - SCREEN_TAB_WIDTH) / PIXEL_WIDTH; // XRESY - to match scree orientation!!!
  //  cursor_row = (byte)p.y / PIXEL_HEIGHT;
    byte border_x = (byte) (320 - CANVAS_WIDTH) / 2;
    byte border_y = (byte) (240 - CANVAS_HEIGHT) / 2; 
    my_debug("canvas raw: x=" + String(p.x) + " y=" + String(p.y));
    cursor_col = (byte)map(p.x-border_x, 0, CANVAS_WIDTH, 0, XRES);
    cursor_row = (byte)map(p.y-border_y, 0, CANVAS_HEIGHT, 0, YRES);
    my_debug("canvas event: x=" + String(cursor_col) + " y=" + String(cursor_row));
  }

  /*
    else if(event == LV_EVENT_LONG_PRESSED){
      en_settings_container = !en_settings_container;
      lv_obj_set_hidden(settings_container, en_settings_container);
  // alternative use talk to close automatically
  //    lv_task_t * task_settings_container = lv_task_create(my_task_container_cb, 10000, LV_TASK_PRIO_MID, NULL); // task to close container after 10 seconds
  //    lv_task_once(task_settings_container); // run task only ones
  ////    lv_task_ready(task_settings_container);
    }
  */
}

static void roller_avr_event_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = lv_event_get_target(e);
  if (code == LV_EVENT_VALUE_CHANGED)
  {
//    SCOPE->avr_cnt = 1U << lv_roller_get_selected(obj);// only works for order: 1, 2, 4, 8, 16
    char buf[4];
    lv_roller_get_selected_str(obj, buf, sizeof(buf));
    SCOPE->avr_cnt = atoi(buf);

    SCOPE->shift = (uint8_t)log2(SCOPE->avr_cnt) / 2 + 0.5;
    //        lv_slider_set_range(slider_color_scale_max, 0 , ADC_RANGE_AD7924 << shift); // we can get up to the full uint16_t bit resolution due to averaging!
    lv_slider_set_range(slider_color_scale_max, 0, ADC_RANGE_AD7924 * SCOPE->avr_cnt); // we can get up to the full uint16_t bit resolution due to averaging!
    color_scale_min = 0;
    //        color_scale_max = ADC_RANGE_AD7924 << shift;
    color_scale_max = ADC_RANGE_AD7924 * SCOPE->avr_cnt;
  }
}


static void roller_colormap_event_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = lv_event_get_target(e);
  if (code == LV_EVENT_VALUE_CHANGED)
  {
    color_map = lv_roller_get_selected(obj);
  }
}

static void roller_chopper_cyc_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = lv_event_get_target(e);
  if (code == LV_EVENT_VALUE_CHANGED)
  {
    uint8_t chopper_div = camera->lookupValidChopper[lv_roller_get_selected(obj)][1];
    uint8_t porch = camera->lookupValidChopper[lv_roller_get_selected(obj)][2]; // PORCH=FPORCH=BPORCH
    uint8_t hsync = camera->lookupValidChopper[lv_roller_get_selected(obj)][3]; 
    camera->WriteMem(ASIC_CAM_CHOP_DIV, chopper_div);
    camera->WriteMem(ASIC_CAM_HPORCH, (porch << 4) | porch);
    camera->WriteMem(ASIC_CAM_HSYNC_CYC, hsync);
  }
}

static void slider_color_scale_max_event_cb(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = lv_event_get_target(e);

  /*Provide some extra space for the value*/
  if (code == LV_EVENT_REFR_EXT_DRAW_SIZE)
  {
    lv_event_set_ext_draw_size(e, 50);
  }
  else if (code == LV_EVENT_DRAW_PART_END)
  {
    lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(e);
    if (dsc->part == LV_PART_INDICATOR)
    {
      char buf[16];
      color_scale_max = (uint16_t)lv_slider_get_value(obj);
      color_scale_min = (uint16_t)lv_slider_get_left_value(obj);
      if(color_scale_max == color_scale_min) color_scale_max++; 
      lv_snprintf(buf, sizeof(buf), "%d - %d", (int)color_scale_min, (int)color_scale_max);
      lv_point_t label_size;
      lv_txt_get_size(&label_size, buf, LV_FONT_DEFAULT, 0, 0, LV_COORD_MAX, 0);
      lv_area_t label_area;
      label_area.x1 = dsc->draw_area->x1 + lv_area_get_width(dsc->draw_area) / 2 - label_size.x / 2;
      label_area.x2 = label_area.x1 + label_size.x;
      label_area.y2 = dsc->draw_area->y1 - 10;
      label_area.y1 = label_area.y2 - label_size.y;

      lv_draw_label_dsc_t label_draw_dsc;
      lv_draw_label_dsc_init(&label_draw_dsc);
      label_draw_dsc.color = lv_color_hex3(0x888);
      lv_draw_label(dsc->draw_ctx, &label_draw_dsc, &label_area, buf, NULL);
    }
  }
}

void SelectPixel(byte PixNum)
{
  byte RowIdx = 7 - int((PixNum - 1) / 8);
  byte ColIdx = 7 - int((PixNum - 1) % 8);
  my_debug("Row: " + String(RowIdx) + ", Col: " + String(ColIdx));
  byte DecoderAddr = (ColIdx << 4) | RowIdx;
  my_debug(String(DecoderAddr));
  //    WriteCmd(BISTSEL, DecoderAddr);
}

static void btnm3_event_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = lv_event_get_target(e);
  if(SCOPE->graph_idx == SCOPE_TYPE_STREAM){
    lv_btnmatrix_clear_btn_ctrl_all(obj, LV_BTNMATRIX_CTRL_CHECKED);
    return;
  } 

  if (code == LV_EVENT_VALUE_CHANGED)
  {
    uint32_t id = lv_btnmatrix_get_selected_btn(obj);
    bool state = !lv_btnmatrix_has_btn_ctrl(obj, lv_btnmatrix_get_selected_btn(obj), LV_BTNMATRIX_CTRL_CHECKED);
    if (id == 0)
    {
      en_scale = state;
      if (en_scale)
      {
        SCOPE->BackupFrame(SCOPE->peak);
      }
    }
    else if (id == 1)
    {
      en_zero = state;
      if (en_zero)
      {
        // SCOPE->ZeroFrame();
        SCOPE->BackupFrame(SCOPE->offset);
        en_zero_save = true; // make sure frame is saved on power off.
        // if(en_zero_save) SCOPE->SaveFrame(SCOPE->offset); 
      }
    }
    else if (id == 2)
    {
      //                FEATURE_BIST->state = !state;
      //                FEATURE_BIST->DoIt();
      if (state){
        SCOPE->graph_idx = SCOPE_TYPE_TIME;
        lv_obj_clear_flag(chart, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(histo, LV_OBJ_FLAG_HIDDEN);
        chart_min=-10;
        chart_max=10;
        lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, chart_min, chart_max);
        lv_canvas_fill_bg(canvas, lv_palette_lighten(LV_PALETTE_GREY, 3), LV_OPA_COVER);
//        lv_canvas_fill_bg(canvas, lv_color_hex(0x000000), LV_OPA_COVER);
      }else{
        SCOPE->graph_idx = SCOPE_TYPE_VIDEO;
        lv_obj_add_flag(chart, LV_OBJ_FLAG_HIDDEN);
      }
      lv_btnmatrix_clear_btn_ctrl(obj, 3, LV_BTNMATRIX_CTRL_CHECKED);
      lv_btnmatrix_clear_btn_ctrl(obj, 4, LV_BTNMATRIX_CTRL_CHECKED);
    }
    else if (id == 3)
    {
      if (state){
        SCOPE->graph_idx = SCOPE_TYPE_FREQ;
//        camera->ReadFrame(SCOPE->raw2, 1);
//        SCOPE->StatsOnFrame(SCOPE->raw2); 
        lv_chart_set_range(histo, LV_CHART_AXIS_PRIMARY_X, SCOPE->min, SCOPE->max);
        lv_obj_clear_flag(histo, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(chart, LV_OBJ_FLAG_HIDDEN);
        histo_max = 50;
        lv_chart_set_range(histo, LV_CHART_AXIS_PRIMARY_Y, 0, histo_max);
        lv_canvas_fill_bg(canvas, lv_palette_lighten(LV_PALETTE_GREY, 3), LV_OPA_COVER);
        lv_coord_t * histo_array = lv_chart_get_y_array(histo, histo_ser);
        for(uint8_t i = 0; i < 13; i++) {
          histo_array[i] = 0;
        }
      }else{
        SCOPE->graph_idx = SCOPE_TYPE_VIDEO;
        lv_obj_add_flag(histo, LV_OBJ_FLAG_HIDDEN);
      }
      lv_btnmatrix_clear_btn_ctrl(obj, 2, LV_BTNMATRIX_CTRL_CHECKED);
      lv_btnmatrix_clear_btn_ctrl(obj, 4, LV_BTNMATRIX_CTRL_CHECKED);
    }
/*    else if (id == 4)
    {
      if (state)
        SCOPE->graph_idx = SCOPE_TYPE_SPEC;
      else
        SCOPE->graph_idx = SCOPE_TYPE_VIDEO;
      lv_btnmatrix_clear_btn_ctrl(obj, 2, LV_BTNMATRIX_CTRL_CHECKED);
      lv_btnmatrix_clear_btn_ctrl(obj, 3, LV_BTNMATRIX_CTRL_CHECKED);
    }
*/
    else if (id == 4)
    {
      FEATURE_MONITOR->state = state;
      FEATURE_MONITOR->DoIt();
    }
  }
}

static void btnm4_event_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = lv_event_get_target(e);
  if (code == LV_EVENT_VALUE_CHANGED)
  {
    uint8_t id = (uint8_t)lv_btnmatrix_get_selected_btn(obj);
    if (id < 8)
    { // Tx Register
      if (ShiftRegTx & (1U << id))
      {
        ShiftRegTx &= ~(1U << id);
        my_debug("Turn OFF bit: " + String(id));
      }
      else
      {
        ShiftRegTx |= 1U << id;
        my_debug("Turn ON bit: " + String(id));
      }
    }
    else if (id == 16)
    { // READ/WRITE button
      // ShiftRegRx = spi_transfer(ShiftRegTx);
      /*          ASIC_SARRAY->tx_data = ShiftRegTx;
                ASIC_SARRAY->DoIt();
                ShiftRegRx = ASIC_SARRAY->rx_data;
      */
      camera->tx_data = ShiftRegTx;
      camera->DoIt();
      ShiftRegRx = camera->rx_data;
      for (uint8_t i = 0; i < 8; i++)
      {
        if (ShiftRegRx & (1U << i))
        {
          lv_btnmatrix_set_btn_ctrl(obj, 15 - i, LV_BTNMATRIX_CTRL_CHECKED);
        }
        else
        {
          lv_btnmatrix_clear_btn_ctrl(obj, 15 - i, LV_BTNMATRIX_CTRL_CHECKED);
        }
      }
    }
    else
    { // ignore
    }
  }
}

static void roller_CMD_event_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = lv_event_get_target(e);
  if (code == LV_EVENT_VALUE_CHANGED)
  {
    switch (lv_roller_get_selected(obj))
    {
    case 0:
      ShiftRegTx = ASIC_CAM_CHIP_ID | (ShiftRegTx & 0x80);
      break;
    case 1:
      ShiftRegTx = ASIC_CAM_CHOP_DIV | (ShiftRegTx & 0x80);
      break;
    case 2:
      ShiftRegTx = ASIC_CAM_PCLK_CYC_LBYTE | (ShiftRegTx & 0x80);
      break;
    case 3:
      ShiftRegTx = ASIC_CAM_PCLK_CYC_HBYTE | (ShiftRegTx & 0x80);
      break;
    case 4:
      ShiftRegTx = ASIC_CAM_HSYNC_CYC | (ShiftRegTx & 0x80);
      break;
    case 5:
      ShiftRegTx = ASIC_CAM_HPORCH | (ShiftRegTx & 0x80);
      break;
    case 6:
      ShiftRegTx = ASIC_CAM_VPORCH | (ShiftRegTx & 0x80);
      break;
    case 7:
      ShiftRegTx = ASIC_CAM_VC_EN | (ShiftRegTx & 0x80);
      break;
    case 8:
      ShiftRegTx = ASIC_CAM_SUBSTRACT_EN | (ShiftRegTx & 0x80);
      break;
    case 9:
      ShiftRegTx = ASIC_CAM_CHOPPER_SELECT | (ShiftRegTx & 0x80);
      break;
    }
    for (uint8_t i = 0; i < 8; i++)
    { // show it
      if (ShiftRegTx & (1U << i))
      {
        lv_btnmatrix_set_btn_ctrl(btnm4, 7 - i, LV_BTNMATRIX_CTRL_CHECKED);
      }
      else
      {
        lv_btnmatrix_clear_btn_ctrl(btnm4, 7 - i, LV_BTNMATRIX_CTRL_CHECKED);
      }
    }
  }
}

static void roller_RW_event_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = lv_event_get_target(e);
  if (code == LV_EVENT_VALUE_CHANGED)
  {
    switch (lv_roller_get_selected(obj))
    {
    case 0:
      ShiftRegTx = ASIC_R | (ShiftRegTx & 0x7F);
      break;
    case 1:
      ShiftRegTx = ASIC_W | (ShiftRegTx & 0x7F);
      break;
    }
    for (uint8_t i = 0; i < 8; i++)
    { // show it
      if (ShiftRegTx & (1U << i))
      {
        lv_btnmatrix_set_btn_ctrl(btnm4, 7 - i, LV_BTNMATRIX_CTRL_CHECKED);
      }
      else
      {
        lv_btnmatrix_clear_btn_ctrl(btnm4, 7 - i, LV_BTNMATRIX_CTRL_CHECKED);
      }
    }
  }
}

int SetupNTP(){
  my_debug("- NTP/RTC");
  configTzTime(NTP_TIMEZONE, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);

int connectionAttempts = 0;
  while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED && connectionAttempts < 10) {
    connectionAttempts++;
    my_debug("Failed to connect to NTP. Retrying...");
    delay(1000); // Wait for 1 second before retrying
  }

  if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
    my_debug("Connected to NTP!");
    // Your code here
  } else {
    my_debug("Failed to connect to NTP within timeout.");
    return(-1);
    // Handle the timeout condition here, e.g., retry or take appropriate action
  }

  time_t t = time(nullptr) + 1; // Advance one second.
  while (t > time(nullptr))
    ; /// Synchronization in seconds

  struct tm *tm;
  tm = localtime(&t);

  RTCtime.Hours = tm->tm_hour;
  RTCtime.Minutes = tm->tm_min;
  RTCtime.Seconds = tm->tm_sec;
  M5.Rtc.SetTime(&RTCtime);

  RTCDate.Year = tm->tm_year + 1900;
  RTCDate.Month = tm->tm_mon + 1;
  RTCDate.Date = tm->tm_mday;
  M5.Rtc.SetDate(&RTCDate);
  MYM5_DATE->DoIt();
  MYM5_DATE->UpdateTable();
  return(0);
}

int SetupAP_STA(){
  WiFi.mode(WIFI_AP_STA); // WIFI_STA
  // if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
  //   my_debug("STA Failed to configure");}
  // }
  WiFi.begin(WIFI_SSID, WIFI_PASSWD);

// Wait for Wi-Fi connection with a timeout of 10 seconds (10000 milliseconds)
  int connectionAttempts = 0;
  while (WiFi.waitForConnectResult() != WL_CONNECTED && connectionAttempts < 10) {
    connectionAttempts++;
    my_debug("Failed to connect to Wi-Fi. Retrying...");
    delay(1000); // Wait for 1 second before retrying
  }

  if (WiFi.status() == WL_CONNECTED) {
    my_debug("Connected to Wi-Fi!");
    // Your code here
  } else {
    my_debug("Failed to connect to Wi-Fi within timeout.");
    return(-1);
    // Handle the timeout condition here, e.g., retry or take appropriate action
  }

  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  my_debug("Connected to " + String(WIFI_SSID));
  my_debug("IP address: " + WiFi.localIP().toString());
  WiFi.setTxPower(WIFI_POWER_11dBm); // adjustabel dbm: 2, 5, 7, 8_5, 11, 13, 15, 17, 18_5, 19, 19_5
  my_debug("Tx Power: " + String(WiFi.getTxPower()));
  my_debug("- Set up AP Mode");
  MYM5_WIFI->DoIt();
  MYM5_WIFI->UpdateTable();

  server.begin();
  server.setNoDelay(true);
  return(0);
}

int SetupAP(){
  my_debug("- Set up AP Mode");
  WiFi.mode(WIFI_AP);
  //WiFi.begin(WIFI_SSID, WIFI_PASSWD);
  WiFi.softAP(WIFI_SSID_AP, WIFI_PASSWD);
  WiFi.setTxPower(WIFI_POWER_11dBm); // adjustabel dbm: 2, 5, 7, 8_5, 11, 13, 15, 17, 18_5, 19, 19_5
  my_debug("Tx Power: " + String(WiFi.getTxPower()));
  my_debug("AP IP address: " + WiFi.softAPIP().toString());
  MYM5_WIFI->DoIt();
  MYM5_WIFI->UpdateTable();

  server.begin();
//  server_http.begin();
  http_server_init();

  server.setNoDelay(true);
  return(0);
}

int SetupOTA(){
  my_debug("- Setup OTA");

  // Port defaults to 3232
  ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname("TicwaveCamera");

  // No authentication by default
  ArduinoOTA.setPassword("work4me+");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]()
               {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      my_debug("Start updating " + type); })
      .onEnd([]()
             { my_debug("\nEnd"); })
      .onProgress([](unsigned int progress, unsigned int total)
                  { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); })
      .onError([](ota_error_t error)
               {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) my_debug("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) my_debug("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) my_debug("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) my_debug("Receive Failed");
      else if (error == OTA_END_ERROR) my_debug("End Failed"); });

  ArduinoOTA.begin();
  return(0);
}

esp_err_t control(httpd_req_t *req) {
    return httpd_resp_send(req, NULL, 0);
}

esp_err_t controlPage(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    return httpd_resp_send(req, NULL, 0);
}

// Function to handle the root URI "/"
esp_err_t rootHandler(httpd_req_t *req) {
    httpd_resp_send(req, "Hello, TicWave Cam!", -1);
    return ESP_OK;
}


////////////////////////////

typedef struct
{
    httpd_req_t *req;
    size_t len;
} jpg_chunking_t;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART = "Content-Type: image/jpg\r\nContent-Length: %u\r\nX-Timestamp: %d.%06d\r\n\r\n";

httpd_handle_t stream_httpd = NULL;
httpd_handle_t camera_httpd = NULL;
static ra_filter_t ra_filter;

static esp_err_t status_handler(httpd_req_t *req)
{
    static char json_response[1];
/*
    static char json_response[1024];
    sensor_t *s = esp_camera_sensor_get();
    char *p = json_response;
    *p++ = '{';

    if(s->id.PID == OV5640_PID || s->id.PID == OV3660_PID){
        for(int reg = 0x3400; reg < 0x3406; reg+=2){
            p+=print_reg(p, s, reg, 0xFFF);//12 bit
        }
        p+=print_reg(p, s, 0x3406, 0xFF);

        p+=print_reg(p, s, 0x3500, 0xFFFF0);//16 bit
        p+=print_reg(p, s, 0x3503, 0xFF);
        p+=print_reg(p, s, 0x350a, 0x3FF);//10 bit
        p+=print_reg(p, s, 0x350c, 0xFFFF);//16 bit

        for(int reg = 0x5480; reg <= 0x5490; reg++){
            p+=print_reg(p, s, reg, 0xFF);
        }

        for(int reg = 0x5380; reg <= 0x538b; reg++){
            p+=print_reg(p, s, reg, 0xFF);
        }

        for(int reg = 0x5580; reg < 0x558a; reg++){
            p+=print_reg(p, s, reg, 0xFF);
        }
        p+=print_reg(p, s, 0x558a, 0x1FF);//9 bit
    } else if(s->id.PID == OV2640_PID){
        p+=print_reg(p, s, 0xd3, 0xFF);
        p+=print_reg(p, s, 0x111, 0xFF);
        p+=print_reg(p, s, 0x132, 0xFF);
    }

    p += sprintf(p, "\"xclk\":%u,", s->xclk_freq_hz / 1000000);
    p += sprintf(p, "\"pixformat\":%u,", s->pixformat);
    p += sprintf(p, "\"framesize\":%u,", s->status.framesize);
    p += sprintf(p, "\"quality\":%u,", s->status.quality);
    p += sprintf(p, "\"brightness\":%d,", s->status.brightness);
    p += sprintf(p, "\"contrast\":%d,", s->status.contrast);
    p += sprintf(p, "\"saturation\":%d,", s->status.saturation);
    p += sprintf(p, "\"sharpness\":%d,", s->status.sharpness);
    p += sprintf(p, "\"special_effect\":%u,", s->status.special_effect);
    p += sprintf(p, "\"wb_mode\":%u,", s->status.wb_mode);
    p += sprintf(p, "\"awb\":%u,", s->status.awb);
    p += sprintf(p, "\"awb_gain\":%u,", s->status.awb_gain);
    p += sprintf(p, "\"aec\":%u,", s->status.aec);
    p += sprintf(p, "\"aec2\":%u,", s->status.aec2);
    p += sprintf(p, "\"ae_level\":%d,", s->status.ae_level);
    p += sprintf(p, "\"aec_value\":%u,", s->status.aec_value);
    p += sprintf(p, "\"agc\":%u,", s->status.agc);
    p += sprintf(p, "\"agc_gain\":%u,", s->status.agc_gain);
    p += sprintf(p, "\"gainceiling\":%u,", s->status.gainceiling);
    p += sprintf(p, "\"bpc\":%u,", s->status.bpc);
    p += sprintf(p, "\"wpc\":%u,", s->status.wpc);
    p += sprintf(p, "\"raw_gma\":%u,", s->status.raw_gma);
    p += sprintf(p, "\"lenc\":%u,", s->status.lenc);
    p += sprintf(p, "\"hmirror\":%u,", s->status.hmirror);
    p += sprintf(p, "\"dcw\":%u,", s->status.dcw);
    p += sprintf(p, "\"colorbar\":%u", s->status.colorbar);
#if CONFIG_LED_ILLUMINATOR_ENABLED
    p += sprintf(p, ",\"led_intensity\":%u", led_duty);
#else
    p += sprintf(p, ",\"led_intensity\":%d", -1);
#endif
#if CONFIG_ESP_FACE_DETECT_ENABLED
    p += sprintf(p, ",\"face_detect\":%u", detection_enabled);
#if CONFIG_ESP_FACE_RECOGNITION_ENABLED
    p += sprintf(p, ",\"face_enroll\":%u,", is_enrolling);
    p += sprintf(p, "\"face_recognize\":%u", recognition_enabled);
#endif
#endif
    *p++ = '}';
    *p++ = 0;

*/
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, json_response, strlen(json_response));
}

static esp_err_t xclk_handler(httpd_req_t *req)
{
    char *buf = NULL;
    char _xclk[32];
/*
    if (parse_get(req, &buf) != ESP_OK) {
        return ESP_FAIL;
    }
    if (httpd_query_key_value(buf, "xclk", _xclk, sizeof(_xclk)) != ESP_OK) {
        free(buf);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    free(buf);

    int xclk = atoi(_xclk);
    log_i("Set XCLK: %d MHz", xclk);

    sensor_t *s = esp_camera_sensor_get();
    int res = s->set_xclk(s, LEDC_TIMER_0, xclk);
    if (res) {
        return httpd_resp_send_500(req);
    }
*/
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, NULL, 0);
}

static int parse_get_var(char *buf, const char * key, int def)
{
    char _int[16];
    if(httpd_query_key_value(buf, key, _int, sizeof(_int)) != ESP_OK){
        return def;
    }
    return atoi(_int);
}

static esp_err_t parse_get(httpd_req_t *req, char **obuf)
{
    char *buf = NULL;
    size_t buf_len = 0;

    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = (char *)malloc(buf_len);
        if (!buf) {
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            *obuf = buf;
            return ESP_OK;
        }
        free(buf);
    }
    httpd_resp_send_404(req);
    return ESP_FAIL;
}

static esp_err_t reg_handler(httpd_req_t *req)
{
    char *buf = NULL;
    char _reg[32];
    char _mask[32];
    char _val[32];

    if (parse_get(req, &buf) != ESP_OK) {
        return ESP_FAIL;
    }
    if (httpd_query_key_value(buf, "reg", _reg, sizeof(_reg)) != ESP_OK ||
        httpd_query_key_value(buf, "mask", _mask, sizeof(_mask)) != ESP_OK ||
        httpd_query_key_value(buf, "val", _val, sizeof(_val)) != ESP_OK) {
        free(buf);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    free(buf);

    uint8_t reg = atoi(_reg);
    int mask = atoi(_mask);
    uint8_t val = atoi(_val);
    log_i("Set Register: reg: 0x%02x, mask: 0x%02x, value: 0x%02x", reg, mask, val);

    //sensor_t *s = esp_camera_sensor_get();
    //int res = s->set_reg(s, reg, mask, val);
    
    camera->WriteMem(ASIC_CAM_CMD | reg, val);
    uint8_t res = 0;

    if (res) {
        return httpd_resp_send_500(req);
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, NULL, 0);
}

static esp_err_t greg_handler(httpd_req_t *req)
{
    char *buf = NULL;
    char _reg[32];
    char _mask[32];
 
    if (parse_get(req, &buf) != ESP_OK) {
        return ESP_FAIL;
    }
    if (httpd_query_key_value(buf, "reg", _reg, sizeof(_reg)) != ESP_OK ||
        httpd_query_key_value(buf, "mask", _mask, sizeof(_mask)) != ESP_OK) {
        free(buf);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    free(buf);

    int reg = atoi(_reg);
    int mask = atoi(_mask);

//   sensor_t *s = esp_camera_sensor_get();
//    int res = s->get_reg(s, reg, mask);
    uint8_t res = camera->ReadMem(ASIC_CAM_CMD | reg);

    if (res < 0) {
        return httpd_resp_send_500(req);
    }
    log_i("Get Register: reg: 0x%02x, mask: 0x%02x, value: 0x%02x", reg, mask, res);

    char buffer[20];
    const char * val = itoa(res, buffer, 10);
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, val, strlen(val));
}

static esp_err_t pll_handler(httpd_req_t *req)
{
    char *buf = NULL;
/*
    if (parse_get(req, &buf) != ESP_OK) {
        return ESP_FAIL;
    }

    int bypass = parse_get_var(buf, "bypass", 0);
    int mul = parse_get_var(buf, "mul", 0);
    int sys = parse_get_var(buf, "sys", 0);
    int root = parse_get_var(buf, "root", 0);
    int pre = parse_get_var(buf, "pre", 0);
    int seld5 = parse_get_var(buf, "seld5", 0);
    int pclken = parse_get_var(buf, "pclken", 0);
    int pclk = parse_get_var(buf, "pclk", 0);
    free(buf);

    log_i("Set Pll: bypass: %d, mul: %d, sys: %d, root: %d, pre: %d, seld5: %d, pclken: %d, pclk: %d", bypass, mul, sys, root, pre, seld5, pclken, pclk);
    sensor_t *s = esp_camera_sensor_get();
    int res = s->set_pll(s, bypass, mul, sys, root, pre, seld5, pclken, pclk);
    if (res) {
        return httpd_resp_send_500(req);
    }
*/
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, NULL, 0);
}

static esp_err_t win_handler(httpd_req_t *req)
{
    char *buf = NULL;
/*
    if (parse_get(req, &buf) != ESP_OK) {
        return ESP_FAIL;
    }

    int startX = parse_get_var(buf, "sx", 0);
    int startY = parse_get_var(buf, "sy", 0);
    int endX = parse_get_var(buf, "ex", 0);
    int endY = parse_get_var(buf, "ey", 0);
    int offsetX = parse_get_var(buf, "offx", 0);
    int offsetY = parse_get_var(buf, "offy", 0);
    int totalX = parse_get_var(buf, "tx", 0);
    int totalY = parse_get_var(buf, "ty", 0);
    int outputX = parse_get_var(buf, "ox", 0);
    int outputY = parse_get_var(buf, "oy", 0);
    bool scale = parse_get_var(buf, "scale", 0) == 1;
    bool binning = parse_get_var(buf, "binning", 0) == 1;
    free(buf);

    log_i("Set Window: Start: %d %d, End: %d %d, Offset: %d %d, Total: %d %d, Output: %d %d, Scale: %u, Binning: %u", startX, startY, endX, endY, offsetX, offsetY, totalX, totalY, outputX, outputY, scale, binning);
    sensor_t *s = esp_camera_sensor_get();
    int res = s->set_res_raw(s, startX, startY, endX, endY, offsetX, offsetY, totalX, totalY, outputX, outputY, scale, binning);
    if (res) {
        return httpd_resp_send_500(req);
    }
*/
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, NULL, 0);
}

static esp_err_t index_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");

//    httpd_resp_set_hdr(req, "Content-Encoding", "gzip"); // if compressed file
//    return httpd_resp_send(req, (const char *)camera_index_html_gz , camera_index_html_gz_len); // if compressed file
    httpd_resp_set_hdr(req, "Content-Encoding", "identity");
    return httpd_resp_send(req, (const char *)camera_index_html, camera_index_html_len); // if uncompressed file
}

static esp_err_t cmd_handler(httpd_req_t *req)
{
    char *buf = NULL;
    char variable[32];
    char value[32];

    if (parse_get(req, &buf) != ESP_OK) {
        return ESP_FAIL;
    }
    if (httpd_query_key_value(buf, "var", variable, sizeof(variable)) != ESP_OK ||
        httpd_query_key_value(buf, "val", value, sizeof(value)) != ESP_OK) {
        free(buf);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    free(buf);

    int val = atoi(value);
    log_i("%s = %d", variable, val);
//    sensor_t *s = esp_camera_sensor_get(); XXX
    int res = 0;

    if (!strcmp(variable, "framesize")) {
//        if (s->pixformat == PIXFORMAT_JPEG) {
//            res = s->set_framesize(s, (framesize_t)val);
//        }
    }
    else if (!strcmp(variable, "xclk")){
      CAMERA_XCLK->cnt = val;
      CAMERA_XCLK->DoIt();
      CAMERA_XCLK->UpdateTable();
      CAMERA_XCLK->UpdateSlider();
      res = 0;
    }
//        res = s->set_quality(s, val);
    else if (!strcmp(variable, "pclk_lb")){
      MEM_PCLK_LB->cnt = val;
      MEM_PCLK_LB->DoIt();
      MEM_PCLK_LB->UpdateTable();
      MEM_PCLK_LB->UpdateSlider();
      res = 0;
    }
//        res = s->set_contrast(s, val);
    else if (!strcmp(variable, "pclk_hb")){
      MEM_PCLK_HB->cnt = val;
      MEM_PCLK_HB->DoIt();
      MEM_PCLK_HB->UpdateTable();
      MEM_PCLK_HB->UpdateSlider();
      res = 0; 
    }
//        res = s->set_brightness(s, val);
    else if (!strcmp(variable, "hsync")){
      MEM_HSYNC->cnt = val;
      MEM_HSYNC->DoIt();
      MEM_HSYNC->UpdateTable();
      MEM_HSYNC->UpdateSlider();
      res = 0;  
    }
//        res = s->set_saturation(s, val);
 //   else if (!strcmp(variable, "gainceiling"))
//        res = s->set_gainceiling(s, (gainceiling_t)val);
//    else if (!strcmp(variable, "colorbar"))
//        res = s->set_colorbar(s, val);
    else if (!strcmp(variable, "vporch")){
      FEATURE_VPORCH->state = val;
      FEATURE_VPORCH->DoIt();
      FEATURE_VPORCH->Update();
      res = 0;  
    }
//        res = s->set_gain_ctrl(s, val);
    else if (!strcmp(variable, "en_vc")){
        FEATURE_VC->state = val;
        FEATURE_VC->DoIt();
        FEATURE_VC->Update();              
        res = 0;
    }
//        res = s->set_exposure_ctrl(s, val);
    else if (!strcmp(variable, "hmirror")){
      FEATURE_HMIRROR->state = val;
      FEATURE_HMIRROR->DoIt();
      FEATURE_HMIRROR->Update();
      res = 0;
    }
//        res = s->set_hmirror(s, val);
  else if (!strcmp(variable, "vflip")){
      FEATURE_VMIRROR->state = val;
      FEATURE_VMIRROR->DoIt();
      FEATURE_VMIRROR->Update();
      res = 0;
  }
  else if (!strcmp(variable, "rotate90")){
      FEATURE_ROTATE90->state = val;
      FEATURE_ROTATE90->DoIt();
      FEATURE_ROTATE90->Update();
      res = 0;
  }
  else if (!strcmp(variable, "persistance")){
      FEATURE_PERSISTANT->state = val;
      FEATURE_PERSISTANT->DoIt();
      FEATURE_PERSISTANT->Update();
      res = 0;
  }
  else if (!strcmp(variable, "autoscale")){
      FEATURE_AUTOSCALE->state = val;
      FEATURE_AUTOSCALE->DoIt();
      FEATURE_AUTOSCALE->Update();
      res = 0;
  }
//        res = s->set_vflip(s, val);
    else if (!strcmp(variable, "subst")){
        FEATURE_SUBST->state = val;
        FEATURE_SUBST->DoIt();
        FEATURE_SUBST->Update();
        res = 0;
    }
//        res = s->set_awb_gain(s, val);
    else if (!strcmp(variable, "chopper_sync")){
        FEATURE_CHOPPER_SYNC->state = val;
        FEATURE_CHOPPER_SYNC->DoIt();
        FEATURE_CHOPPER_SYNC->Update();
        res = 0;
    }
//        res = s->set_agc_gain(s, val);
//    else if (!strcmp(variable, "aec_value"))
//        res = s->set_aec_value(s, val);
//    else if (!strcmp(variable, "aec2"))
//        res = s->set_aec2(s, val);
//    else if (!strcmp(variable, "dcw"))
//        res = s->set_dcw(s, val);
    else if (!strcmp(variable, "zero")){
      en_zero = val;
      if (en_zero)
      {
        SCOPE->BackupFrame(SCOPE->offset);
//        lv_btnmatrix_set_btn_ctrl(btnm3, 1, LV_BTNMATRIX_CTRL_CHECKED);
      } 
      //else lv_btnmatrix_clear_btn_ctrl(btnm3, 1, LV_BTNMATRIX_CTRL_CHECKED);
    }
    else if (!strcmp(variable, "peak")){
      en_scale = val;
      if (en_scale)
      {
        SCOPE->BackupFrame(SCOPE->peak);
        //lv_btnmatrix_set_btn_ctrl(btnm3, 0, LV_BTNMATRIX_CTRL_CHECKED);
      }
      //else lv_btnmatrix_clear_btn_ctrl(btnm3, 0, LV_BTNMATRIX_CTRL_CHECKED);
    } 
//        res = s->set_bpc(s, val);
//    else if (!strcmp(variable, "wpc"))
//        res = s->set_wpc(s, val);
//    else if (!strcmp(variable, "raw_gma"))
//        res = s->set_raw_gma(s, val);
//    else if (!strcmp(variable, "lenc"))
//        res = s->set_lenc(s, val);
    else if (!strcmp(variable, "special_effect")){
      lv_roller_set_selected(roller_colormap, val, LV_ANIM_ON); // select
      lv_event_send(roller_colormap, LV_EVENT_VALUE_CHANGED, NULL);
}
    else if (!strcmp(variable, "chopper_cyc")){
      lv_roller_set_selected(roller_chopper_cyc, val, LV_ANIM_ON); // select
      lv_event_send(roller_chopper_cyc, LV_EVENT_VALUE_CHANGED, NULL);
    }
    else if (!strcmp(variable, "averages")){
      lv_roller_set_selected(roller_avr, val, LV_ANIM_ON); // select
      lv_event_send(roller_avr, LV_EVENT_VALUE_CHANGED, NULL);
    }
//        res = s->set_wb_mode(s, val);
//    else if (!strcmp(variable, "ae_level"))
//        res = s->set_ae_level(s, val);
#if CONFIG_LED_ILLUMINATOR_ENABLED
    else if (!strcmp(variable, "led_intensity")) {
        led_duty = val;
        if (isStreaming)
//            enable_led(true);
    }
#endif

#if CONFIG_ESP_FACE_DETECT_ENABLED
    else if (!strcmp(variable, "face_detect")) {
        detection_enabled = val;
#if CONFIG_ESP_FACE_RECOGNITION_ENABLED
        if (!detection_enabled) {
            recognition_enabled = 0;
        }
#endif
    }
#if CONFIG_ESP_FACE_RECOGNITION_ENABLED
    else if (!strcmp(variable, "face_enroll")){
        is_enrolling = !is_enrolling;
        log_i("Enrolling: %s", is_enrolling?"true":"false");
    }
    else if (!strcmp(variable, "face_recognize")) {
        recognition_enabled = val;
        if (recognition_enabled) {
            detection_enabled = val;
        }
    }
#endif
#endif
    else {
        log_i("Unknown command: %s", variable);
        res = -1;
    }

    if (res < 0) {
        return httpd_resp_send_500(req);
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, NULL, 0);
}

static size_t jpg_encode_stream(void *arg, size_t index, const void *data, size_t len)
{
    jpg_chunking_t *j = (jpg_chunking_t *)arg;
    if (!index)
    {
        j->len = 0;
    }
    if (httpd_resp_send_chunk(j->req, (const char *)data, len) != ESP_OK)
    {
        return 0;
    }
    j->len += len;
    return len;
}

static esp_err_t capture_handler(httpd_req_t *req)
{    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
    int64_t fr_start = esp_timer_get_time();
#endif
    //fb = esp_camera_fb_get();
      fb = SCOPE->FbGet(OR_DEFAULT);
      SCOPE->ImageProcessing(fb, SCOPE->offset, SCOPE->peak, true);
    if (!fb)
    {
        log_e("Camera capture failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    char ts[32];
    snprintf(ts, 32, "%ld.%06ld", fb->timestamp.tv_sec, fb->timestamp.tv_usec);
    httpd_resp_set_hdr(req, "X-Timestamp", (const char *)ts);
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
        size_t fb_len = 0;
#endif

        if (fb->format == PIXFORMAT_JPEG)
        {
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
            fb_len = fb->len;
#endif
            res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
        }
        else
        {
            jpg_chunking_t jchunk = {req, 0};
            res = frame2jpg_cb(fb, 80, jpg_encode_stream, &jchunk) ? ESP_OK : ESP_FAIL;
            httpd_resp_send_chunk(req, NULL, 0);
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
            fb_len = jchunk.len;
#endif
        }
        esp_camera_fb_return(fb);
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
        int64_t fr_end = esp_timer_get_time();
        log_i("JPG: %uB %ums", (uint32_t)(fb_len), (uint32_t)((fr_end - fr_start) / 1000));
#endif
        return res;
}

#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
static int ra_filter_run(ra_filter_t *filter, int value)
{
    if (!filter->values)
    {
        return value;
    }
    filter->sum -= filter->values[filter->index];
    filter->values[filter->index] = value;
    filter->sum += filter->values[filter->index];
    filter->index++;
    filter->index = filter->index % filter->size;
    if (filter->count < filter->size)
    {
        filter->count++;
    }
    return filter->sum / filter->count;
}
#endif

static esp_err_t stream_handler(httpd_req_t *req)
{
    SCOPE->graph_idx = SCOPE_TYPE_STREAM;
    show_popup(lv_layer_sys(), "Streaming wifi"); 

   camera_fb_t *fb = NULL;
    struct timeval _timestamp;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t *_jpg_buf = NULL;
    char *part_buf[128];
#if CONFIG_ESP_FACE_DETECT_ENABLED
    #if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
        bool detected = false;
        int64_t fr_ready = 0;
        int64_t fr_recognize = 0;
        int64_t fr_encode = 0;
        int64_t fr_face = 0;
        int64_t fr_start = 0;
    #endif
    int face_id = 0;
    size_t out_len = 0, out_width = 0, out_height = 0;
    uint8_t *out_buf = NULL;
    bool s = false;
#if TWO_STAGE
    HumanFaceDetectMSR01 s1(0.1F, 0.5F, 10, 0.2F);
    HumanFaceDetectMNP01 s2(0.5F, 0.3F, 5);
#else
    HumanFaceDetectMSR01 s1(0.3F, 0.5F, 10, 0.2F);
#endif
#endif

    static int64_t last_frame = 0;
    if (!last_frame)
    {
        last_frame = esp_timer_get_time();
    }

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if (res != ESP_OK)
    {
        return res;
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "X-Framerate", "60");

#if CONFIG_LED_ILLUMINATOR_ENABLED
    enable_led(true);
    isStreaming = true;
#endif

    while (true)
    {
#if CONFIG_ESP_FACE_DETECT_ENABLED
    #if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
        detected = false;
    #endif
        face_id = 0;
#endif

  uint8_t orientation;
  if(FEATURE_HMIRROR->state && FEATURE_VMIRROR->state) orientation = OR_ROTATE90_LEFT;
  else if(FEATURE_VMIRROR->state) orientation = OR_VMIRROR;
  else if(FEATURE_HMIRROR->state) orientation = OR_HMIRROR;
  else if(FEATURE_ROTATE90->state) orientation = OR_ROTATE90_RIGHT;
  else orientation = OR_DEFAULT;

        fb = SCOPE->FbGet(orientation);

//for(int i =0; i<4; i++){
//  ESP_LOGD(TAG,"main:%d", fb->buf[i]);
//}
        SCOPE->ImageProcessing(fb, SCOPE->offset, SCOPE->peak, true);
        if (!fb)
        {
            log_e("Camera capture failed");
            res = ESP_FAIL;
        }
        else
        {
            _timestamp.tv_sec = fb->timestamp.tv_sec;
            _timestamp.tv_usec = fb->timestamp.tv_usec;
#if CONFIG_ESP_FACE_DETECT_ENABLED
    #if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
            fr_start = esp_timer_get_time();
            fr_ready = fr_start;
            fr_encode = fr_start;
            fr_recognize = fr_start;
            fr_face = fr_start;
    #endif
            if (!detection_enabled || fb->width > 400)
            {
#endif
                if (fb->format != PIXFORMAT_JPEG)
                {
                    bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
//                    bool jpeg_converted = fmt2jpg(fb->buf, fb->len, fb->width, fb->height, PIXFORMAT_RGB565, 80, &_jpg_buf, &_jpg_buf_len);
                    esp_camera_fb_return(fb);
                    fb = NULL;
                    if (!jpeg_converted)
                    {
                        log_e("JPEG compression failed");
                        res = ESP_FAIL;
                    }
                }
                else
                {
                    _jpg_buf_len = fb->len;
                    _jpg_buf = fb->buf;
                }
#if CONFIG_ESP_FACE_DETECT_ENABLED
            }
            else
            {
                if (fb->format == PIXFORMAT_RGB565
#if CONFIG_ESP_FACE_RECOGNITION_ENABLED
                    && !recognition_enabled
#endif
                ){
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
                    fr_ready = esp_timer_get_time();
#endif
#if TWO_STAGE
                    std::list<dl::detect::result_t> &candidates = s1.infer((uint16_t *)fb->buf, {(int)fb->height, (int)fb->width, 3});
                    std::list<dl::detect::result_t> &results = s2.infer((uint16_t *)fb->buf, {(int)fb->height, (int)fb->width, 3}, candidates);
#else
                    std::list<dl::detect::result_t> &results = s1.infer((uint16_t *)fb->buf, {(int)fb->height, (int)fb->width, 3});
#endif
#if CONFIG_ESP_FACE_DETECT_ENABLED && ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
                    fr_face = esp_timer_get_time();
                    fr_recognize = fr_face;
#endif
                    if (results.size() > 0) {
                        fb_data_t rfb;
                        rfb.width = fb->width;
                        rfb.height = fb->height;
                        rfb.data = fb->buf;
                        rfb.bytes_per_pixel = 2;
                        rfb.format = FB_RGB565;
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
                        detected = true;
#endif
                        draw_face_boxes(&rfb, &results, face_id);
                    }
//                    s = fmt2jpg(fb->buf, fb->len, fb->width, fb->height, PIXFORMAT_RGB565, 80, &_jpg_buf, &_jpg_buf_len);
                    s = fmt2jpg(fb->buf, fb->len, fb->width, fb->height, PIXFORMAT_GRAYSCALE, 80, &_jpg_buf, &_jpg_buf_len);
                    ESP_LOGD("format: ", fb->format);
                    esp_camera_fb_return(fb);
                    fb = NULL;
                    if (!s) {
                        log_e("fmt2jpg failed");
                        res = ESP_FAIL;
                    }
#if CONFIG_ESP_FACE_DETECT_ENABLED && ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
                    fr_encode = esp_timer_get_time();
#endif
                } else
                {
                    out_len = fb->width * fb->height * 3;
                    out_width = fb->width;
                    out_height = fb->height;
                    out_buf = (uint8_t*)malloc(out_len);
                    if (!out_buf) {
                        log_e("out_buf malloc failed");
                        res = ESP_FAIL;
                    } else {
                        s = fmt2rgb888(fb->buf, fb->len, fb->format, out_buf);
                        esp_camera_fb_return(fb);
                        fb = NULL;
                        if (!s) {
                            free(out_buf);
                            log_e("To rgb888 failed");
                            res = ESP_FAIL;
                        } else {
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
                            fr_ready = esp_timer_get_time();
#endif

                            fb_data_t rfb;
                            rfb.width = out_width;
                            rfb.height = out_height;
                            rfb.data = out_buf;
                            rfb.bytes_per_pixel = 3;
                            rfb.format = FB_BGR888;

#if TWO_STAGE
                            std::list<dl::detect::result_t> &candidates = s1.infer((uint8_t *)out_buf, {(int)out_height, (int)out_width, 3});
                            std::list<dl::detect::result_t> &results = s2.infer((uint8_t *)out_buf, {(int)out_height, (int)out_width, 3}, candidates);
#else
                            std::list<dl::detect::result_t> &results = s1.infer((uint8_t *)out_buf, {(int)out_height, (int)out_width, 3});
#endif

#if CONFIG_ESP_FACE_DETECT_ENABLED && ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
                            fr_face = esp_timer_get_time();
                            fr_recognize = fr_face;
#endif

                            if (results.size() > 0) {
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
                                detected = true;
#endif
#if CONFIG_ESP_FACE_RECOGNITION_ENABLED
                                if (recognition_enabled) {
                                    face_id = run_face_recognition(&rfb, &results);
    #if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
                                    fr_recognize = esp_timer_get_time();
    #endif
                                }
#endif
                                draw_face_boxes(&rfb, &results, face_id);
                            }
                            s = fmt2jpg(out_buf, out_len, out_width, out_height, PIXFORMAT_RGB888, 90, &_jpg_buf, &_jpg_buf_len);
                            free(out_buf);
                            if (!s) {
                                log_e("fmt2jpg failed");
                                res = ESP_FAIL;
                            }
#if CONFIG_ESP_FACE_DETECT_ENABLED && ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
                            fr_encode = esp_timer_get_time();
#endif
                        }
                    }
                }
            }
#endif
        }
        if (res == ESP_OK)
        {
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        if (res == ESP_OK)
        {
            size_t hlen = snprintf((char *)part_buf, 128, _STREAM_PART, _jpg_buf_len, _timestamp.tv_sec, _timestamp.tv_usec);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if (res == ESP_OK)
        {
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if (fb)
        {
            esp_camera_fb_return(fb);
            fb = NULL;
            _jpg_buf = NULL;
        }
        else if (_jpg_buf)
        {
            free(_jpg_buf);
            _jpg_buf = NULL;
        }
        if (res != ESP_OK)
        {
            log_e("Send frame failed");
            break;
        }
        int64_t fr_end = esp_timer_get_time();

#if CONFIG_ESP_FACE_DETECT_ENABLED && ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
        int64_t ready_time = (fr_ready - fr_start) / 1000;
        int64_t face_time = (fr_face - fr_ready) / 1000;
        int64_t recognize_time = (fr_recognize - fr_face) / 1000;
        int64_t encode_time = (fr_encode - fr_recognize) / 1000;
        int64_t process_time = (fr_encode - fr_start) / 1000;
#endif

        int64_t frame_time = fr_end - last_frame;
        frame_time /= 1000;
        last_frame = fr_end; // somehow I missed this here!!!
   //     uint32_t avg_frame_time = ra_filter_run(&ra_filter, frame_time);
   //     log_i("MJPG: %uB %ums (%.1ffps), AVG: %ums (%.1ffps)",
   //              (uint32_t)(_jpg_buf_len),
   //              (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time,
   //              avg_frame_time, 1000.0 / avg_frame_time
   //     );
    }

#if CONFIG_LED_ILLUMINATOR_ENABLED
    isStreaming = false;
    enable_led(false);
#endif
    delete_popup(lv_layer_sys());
    SCOPE->graph_idx = SCOPE_TYPE_VIDEO;
    return res;
}


static esp_err_t http_server_init() {

    httpd_uri_t index_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = index_handler,
        .user_ctx = NULL,
        .is_websocket = true,
        .handle_ws_control_frames = false,
        .supported_subprotocol = NULL
    };

    httpd_uri_t status_uri = {
        .uri = "/status",
        .method = HTTP_GET,
        .handler = status_handler,
        .user_ctx = NULL,
        .is_websocket = true,
        .handle_ws_control_frames = false,
        .supported_subprotocol = NULL
    };

    httpd_uri_t cmd_uri = {
        .uri = "/control",
        .method = HTTP_GET,
        .handler = cmd_handler,
        .user_ctx = NULL,
        .is_websocket = true,
        .handle_ws_control_frames = false,
        .supported_subprotocol = NULL
    };

    httpd_uri_t capture_uri = {
        .uri = "/capture",
        .method = HTTP_GET,
        .handler = capture_handler,
        .user_ctx = NULL,
        .is_websocket = true,
        .handle_ws_control_frames = false,
        .supported_subprotocol = NULL
    };

    httpd_uri_t stream_uri = {
        .uri = "/stream",
        .method = HTTP_GET,
        .handler = stream_handler,
        .user_ctx = NULL,
        .is_websocket = true,
        .handle_ws_control_frames = false,
        .supported_subprotocol = NULL
    };

    httpd_uri_t bmp_uri = {
        .uri = "/bmp",
        .method = HTTP_GET,
        .handler = capture_handler,
        .user_ctx = NULL,
        .is_websocket = true,
        .handle_ws_control_frames = false,
        .supported_subprotocol = NULL
    };

    httpd_uri_t xclk_uri = {
        .uri = "/xclk",
        .method = HTTP_GET,
        .handler = xclk_handler,
        .user_ctx = NULL,
        .is_websocket = true,
        .handle_ws_control_frames = false,
        .supported_subprotocol = NULL
    };

    httpd_uri_t reg_uri = {
        .uri = "/reg",
        .method = HTTP_GET,
        .handler = reg_handler,
        .user_ctx = NULL,
        .is_websocket = true,
        .handle_ws_control_frames = false,
        .supported_subprotocol = NULL
    };

    httpd_uri_t greg_uri = {
        .uri = "/greg",
        .method = HTTP_GET,
        .handler = greg_handler,
        .user_ctx = NULL,
        .is_websocket = true,
        .handle_ws_control_frames = false,
        .supported_subprotocol = NULL
    };

    httpd_uri_t pll_uri = {
        .uri = "/pll",
        .method = HTTP_GET,
        .handler = pll_handler,
        .user_ctx = NULL,
        .is_websocket = true,
        .handle_ws_control_frames = false,
        .supported_subprotocol = NULL
    };

    httpd_uri_t win_uri = {
        .uri = "/resolution",
        .method = HTTP_GET,
        .handler = win_handler,
        .user_ctx = NULL,
        .is_websocket = true,
        .handle_ws_control_frames = false,
        .supported_subprotocol = NULL
    };

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 16;
    
    if (httpd_start(&camera_httpd, &config) == ESP_OK)
    {
        httpd_register_uri_handler(camera_httpd, &index_uri);
        httpd_register_uri_handler(camera_httpd, &cmd_uri);
        httpd_register_uri_handler(camera_httpd, &status_uri);
        httpd_register_uri_handler(camera_httpd, &capture_uri);
        httpd_register_uri_handler(camera_httpd, &bmp_uri);

        httpd_register_uri_handler(camera_httpd, &xclk_uri);
        httpd_register_uri_handler(camera_httpd, &reg_uri);
        httpd_register_uri_handler(camera_httpd, &greg_uri);
        httpd_register_uri_handler(camera_httpd, &pll_uri);
        httpd_register_uri_handler(camera_httpd, &win_uri);
    }

    config.server_port += 1;
    config.ctrl_port += 1;
    log_i("Starting stream server on port: '%d'", config.server_port);
    if (httpd_start(&stream_httpd, &config) == ESP_OK)
    {
        httpd_register_uri_handler(stream_httpd, &stream_uri);
    }

    return ESP_OK;
}

int SetupWifi()
{
  my_debug("- Setup WIFI");
//  if(SetupAP_STA()) return(-1);
  if(SetupAP()) return(-1);
  // if(SetupNTP()) return(-1);
  #ifdef WITH_OTA 
  if(SetupOTA()) return(-1);
  #endif
  return(0);
}

uint32_t ColorMap32bitRGB(byte map, uint32_t gray, uint32_t min, uint32_t max)
{
  uint32_t color;
  switch (map)
  {
  case GRAYSCALE:
    color = ColorMapGray(gray, min, max);
    break;
  case JETSCALE:
    color = ColorMapJet(gray, min, max);
    break;
  case COOLSCALE:
    color = ColorMapCool(gray, min, max);
    break;
  case SPRINGSCALE:
    color = ColorMapSpring(gray, min, max);
    break;
  case AUTUMNSCALE:
    color = ColorMapAutumn(gray, min, max);
    break;
  case WINTERSCALE:
    color = ColorMapWinter(gray, min, max);
    break;
  default:
    color = 0x009d8b;
    break;
  }
  return (color);
}

// function to actually bit shift the data byte out
uint8_t spi_transfer(uint8_t tx_data)
{
  if (!simulate)
  {

    uint32_t rx_data = 0;
    /*
      uint8_t bits=8;
      SPI.beginTransaction(SPISettings(SARRAY_SPI_Clock, LSBFIRST, SPI_MODE0)); // MSBFIRST not working
      digitalWrite(GPIO_SARRAY_SS, LOW); //pull SS low to prep other end for transfer
      SPI.transferBits(tx_data,&rx_data, bits);
      digitalWrite(GPIO_SARRAY_SS, HIGH); //pull ss high to signify end of data transfer
      SPI.endTransaction();
    */
    return ((uint8_t)rx_data & 0xFF);
  }
  else
  {
    return (random(63));
  }
}

// flip byte because ESP32 SPI lib can not shift out MSBFIRST
uint8_t reverse(uint8_t b)
{
  b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
  b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
  b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
  return b;
}

void show_popup(lv_obj_t *parent, const char *message) {
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, message);
    lv_obj_center(label);
    lv_obj_set_style_bg_opa(label, LV_OPA_50, 0);
    lv_obj_set_style_bg_color(label, lv_color_black(), 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_pad_all(label, 20, 0);
    // Set a unique identifier as user data (could be any pointer or value)
    lv_obj_set_user_data(label, (void*)0x12345678); // Example unique identifier
    lv_task_handler(); // draw rightaway
}

void delete_popup(lv_obj_t *parent) {
    uint32_t index = 0; // Start with the first child
    lv_obj_t *child = lv_obj_get_child(parent, index);
    while (child) {
        // If the child matches our criteria (e.g., via user data or a unique style property)
        if (lv_obj_get_user_data(child) == (void*)0x12345678) {
            lv_obj_del_async(child); // Delete the popup
            break; // Assuming only one popup matches this condition at a time
        }
        // Move to the next child
        index++;
        child = lv_obj_get_child(parent, index);
    }
}

static void btn_off_event_handler(lv_event_t *event)
{
  if(en_zero_save) SCOPE->SaveFrame(SCOPE->offset);
  M5.Axp.PowerOff();
}

void my_timer_cb(lv_timer_t *timer)
{

  if (FEATURE_MONITOR->state && (lv_tabview_get_tab_act(tabview) == (tab_count-1)))
  {
    lv_obj_clear_flag(perf_monitor.perf_label, LV_OBJ_FLAG_HIDDEN);
  }
  else
  {
    lv_obj_add_flag(perf_monitor.perf_label, LV_OBJ_FLAG_HIDDEN);
    // return; // in case this function is unwanted...
  }

  if (FEATURE_MONITOR->state)
  {
    my_debug("timer");
    MYM5_BAT->DoIt();
    MYM5_BAT->UpdateTable();
    //  MYM5_WIFI->DoIt();
    //  MYM5_WIFI->UpdateTable();
    MYM5_AC->DoIt();
    MYM5_AC->UpdateTable();
    MYM5_CHARGE->DoIt();
    MYM5_CHARGE->UpdateTable();

 #if CONFIG_MONITOR_VERBOSE
    TEMP_CAMERA->DoIt();
    TEMP_CAMERA->UpdateTable();
    ADC_TEMP->DoIt();
    ADC_TEMP->UpdateTable();
    MYM5_TEMP_APX192->DoIt();
    MYM5_TEMP_APX192->UpdateTable();
    MYM5_IPS->DoIt();
    MYM5_IPS->UpdateTable();
    MYM5_VBUS->DoIt();
    MYM5_VBUS->UpdateTable();
    MYM5_DATE->DoIt();
    MYM5_DATE->UpdateTable();
#endif

  if (lv_tabview_get_tab_act(tabview) == (tab_count-1))
  {
    float noise;
    if(SCOPE->graph_idx ==  SCOPE_TYPE_VIDEO){
      uint8_t noise_frame[XRES*YRES*4];
      SCOPE->BackupFrame(noise_frame);
      camera_fb_t *fb = SCOPE->FbGet(OR_DEFAULT);
      bool en_zero_prev = en_zero;
      en_zero = 1;
      SCOPE->ImageProcessing(fb, noise_frame, SCOPE->peak, false);
      en_zero = en_zero_prev;
      SCOPE->StatsOnFrame(fb->buf); 
      esp_camera_fb_return(fb);
      noise = SCOPE->stdev;
      fb = SCOPE->FbGet(OR_DEFAULT);
      SCOPE->ImageProcessing(fb, SCOPE->offset, SCOPE->peak, false);
      esp_camera_fb_return(fb);
      perf_monitor.cnt = SCOPE->cnt;
      SCOPE->StatsOnFrame(fb->buf);       
    }
    uint32_t fps = 0;
    if (perf_monitor.elaps_sum)
      fps = (uint32_t)(1000 * perf_monitor.frame_cnt) / perf_monitor.elaps_sum;
    lv_label_set_text_fmt(perf_monitor.perf_label, "#%dx%d: %d @ SNR:%.1fdB\n%dfps %d\xee\x92\xb8 %d\xee\x93\x82 %.2f\xef\x93\xb3 std:%.2f", cursor_row, cursor_col, perf_monitor.cnt, 20*log10(perf_monitor.cnt/noise), fps, SCOPE->min, SCOPE->max, SCOPE->mean, noise);
  }
  }
}

void MySerialEvent()
{
  MuxSerialToWifi = Serial.available() ? false : true;
  while (Serial.available())
  {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    currentLine += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n')
    {
      stringComplete = true;
      // my_debug("MySerialEvent: " + currentLine);
    }
  }
}

void MyWifiEvent()
{
  client = server.available();
  if (client)
  {
    delay(100); // too slow response from LCARS so add delay
    my_debug("- server.available");
    MuxSerialToWifi = client ? true : false;
    while (client.available())
    {
      delay(100);
      my_debug("- client.available");
      // get the new byte:
      char inChar = (char)client.read();
      // add it to the inputString:
      currentLine += inChar;
      // if the incoming character is a newline, set a flag so the main loop can
      // do something about it:
      if (inChar == '\n')
      {
        stringComplete = true;
        my_debug("- MyWifiEvent: " + currentLine);
      }
    }
  }
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
  send("Listing directory: " + String(dirname) + "\n");

  File root = fs.open(dirname);
  if (!root)
  {
    send("Failed to open directory\n");
    return;
  }
  if (!root.isDirectory())
  {
    send("Not a directory\n");
    return;
  }

  File file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
      send("  DIR : " + String(file.name()) + "\n");
      if (levels)
      {
        listDir(fs, file.path(), levels - 1);
      }
    }
    else
    {
      send("  FILE: " + String(file.name()) + "\n");
      send("  SIZE: " + String(file.size()) + "\n");
    }
    file = root.openNextFile();
  }
  send("END\n");
  //    if(MuxSerialToWifi) client.stop();
}

void readFile(fs::FS &fs, char *path)
{
  //Serial.print("Reading file: " + String(path) + "\n");

  File file = fs.open(path);
  if (!file)
  {
    //  Serial.print("Failed to open file for reading\n");
    return;
  }

  // Serial.print("Read from file: \n");
  while (file.available())
  {
//    MuxSerialToWifi ? client.write(file.read()) : Serial.write(file.read());
    Serial.write(file.read());
  }
  // Serial.print("Finished reading.\n");
  file.close();
//  if (MuxSerialToWifi)
//    client.stop();
}

void deleteFile(fs::FS &fs, char *path)
{
  Serial.printf("Deleting file: %s\n", path);
  if (fs.remove(path))
  {
    my_debug("File deleted");
  }
  else
  {
    my_debug("Delete failed");
  }
}

void send(String message)
{
//  MuxSerialToWifi ? client.print(message) : Serial.print(message);
  MuxSerialToWifi ? client.write((const uint8_t*)message.c_str(), message.length()) : Serial.print(message);
}

int CheckTcpReceive()
{
  MySerialEvent();
  MyWifiEvent();
  if (stringComplete)
  {
    CmdReceived->SplitLine(currentLine);
    CmdReceived->DoIt();
    //Serial.print(currentLine);
    currentLine = "";
    stringComplete = false;
  }
  return 0;
}

//=====================================================================
void loop()
{
  M5.update();
  //M5.Axp.AXPInState(); // could be cheched from time to time to monitor USB connections
  // my_debug("loop");
   if(SCOPE->graph_idx != SCOPE_TYPE_STREAM){lv_timer_handler();}
  // delay(5);
  CheckTcpReceive();
  vTaskDelay(1); // because I received debug errors on serial monitor... check

//for(int i=0; i<33*33; i++){
//  size_t idx = ((33*33-1-i)/33) *33 + (32 - (33*33-1-i)%33);  
  // note: i%33=col i/33=row
//  ESP_LOGE("test","i=%d row(i/33)=%d col(ipercent33)=%d idx=%d", i, i/33, i%33, idx);
//}

#ifdef WITH_OTA
  if(FEATURE_WIFI->state){ArduinoOTA.handle();};
#endif

  //  delay(1);
 

//  switch (lv_tabview_get_tab_act(tabview))
//  {
//  case 0:
    //    PlotTimeDomainExt();
//    break;
//  case 1:
    //    PlotSweepExt(0);
//    break;
//  case 2: // edit menue
          //   PlotFrequencyDomain(FFT_TYPE_NORMAL);
//    break;
//  case 3:
    //  PlotSpectrogram();
//    break;
if (lv_tabview_get_tab_act(tabview) == (tab_count -1)){ // video should be always on the last tab
      SCOPE->DoIt();
      if(SCOPE->graph_idx ==  SCOPE_TYPE_VIDEO){
      
      perf_monitor.cnt = SCOPE->cnt;
      perf_monitor.frame_cnt += SCOPE->avr_cnt;
      perf_monitor.elaps_sum += millis() - perf_monitor.perf_last_time;
      if (perf_monitor.elaps_sum > 1000)
      {
        perf_monitor.frame_cnt = 0;
        perf_monitor.elaps_sum = 0;
      }
      perf_monitor.perf_last_time = millis();
    }
    //else if(SCOPE->graph_idx ==  SCOPE_TYPE_STREAM){
      //camera->ReadFrame(SCOPE->raw2, SCOPE->avr_cnt);
      //SCOPE->ImageProcessing();
    //}
  }
}
//=====================================================================
//=====================================================================
// Initialize the input device driver
void init_touch()
{
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);
}

//=====================================================================
// Read the touchpad --------------------------------------------------
void my_touchpad_read(lv_indev_drv_t *indev_driver,
                      lv_indev_data_t *data)
{
  TouchPoint_t pos = M5.Touch.getPressPoint();
  bool touched = (pos.x == -1) ? false : true;
  if (!touched)
  {
    data->state = LV_INDEV_STATE_REL;
  }
  else
  {
    data->state = LV_INDEV_STATE_PR;
    data->point.x = pos.x;
    data->point.y = pos.y;
    //    Serial.print(pos.x);Serial.print(" , ");my_debug(pos.y);
  }
}
//=====================================================================
//=====================================================================
static void btn_event_cb(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *btn = lv_event_get_target(e);
  if (code == LV_EVENT_CLICKED)
  {
    static uint8_t cnt = 0;
    cnt++;
    // Get the first child of the button which is the label and change its text
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    lv_label_set_text_fmt(label, "Button: %d", cnt);
  }
}

//=====================================================================
// Create a button with a label and react on click event.
void lv_example_get_started_1(void)
{
  // Add a button the current screen
  lv_obj_t *btn = lv_btn_create(lv_scr_act());
  lv_obj_set_size(btn, 120, 50); // Set its size
  lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);
  // Assign a callback to the button
  lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, NULL);
  lv_obj_t *label = lv_label_create(btn); // Add a label to the button
  lv_label_set_text(label, "Button");     // Set the labels text
  lv_obj_center(label);
}
//=====================================================================
//=====================================================================
// Initialize the display -------------------------------------------
void init_display()
{
  static lv_disp_drv_t disp_drv;                    // Descriptor of a display driver
  lv_disp_drv_init(&disp_drv);                      // Basic initialization
  disp_drv.flush_cb = my_disp_flush;                // Set your driver function
  disp_drv.draw_buf = &draw_buf;                    // Assign the buffer
  disp_drv.hor_res = LCD_WIDTH;                     // horizontal resolution
  disp_drv.ver_res = LCD_HEIGHT;                    // vertical resolution
  lv_disp_drv_register(&disp_drv);                  // Finally register the driver
  lv_disp_set_bg_color(NULL, lv_color_hex(0x0000)); // background black
}

//=====================================================================
// Display flushing
void my_disp_flush(lv_disp_drv_t *disp,
                   const lv_area_t *area, lv_color_t *color_p)
{
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  M5.Lcd.startWrite();
  M5.Lcd.setAddrWindow(area->x1, area->y1, w, h);
  M5.Lcd.pushColors(&color_p->full, w * h, true);
  // tft.writePixelsDMA((lgfx::swap565_t*)&color_p->full,w*h); // LVGL
  M5.Lcd.endWrite();
  lv_disp_flush_ready(disp);
}
//=====================================================================
