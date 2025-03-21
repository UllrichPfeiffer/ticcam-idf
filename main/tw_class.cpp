// TicWave GmbH, U. Pfeiffer 15.3.2023
// Code related to the serial communication

#include <Arduino.h>
#include <M5Core2.h>
#include <lvgl.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <vector>
#include <Crypto.h>
#include <SHA256.h>
#include "esp_log.h"
#include "colormap.h"
#include "cam_filter.h"
#include "arduinoFFT.h"
#include "MCP466_DigitalPot.h"
#include "XClk.h"
#include "driver/ledc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "time.h"
#include "sys/time.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "SdFat.h"
#include <cJSON.h>
#include "main.h"
#include "tw_class.h"
#include "global_var.h"

#define TAG "tw_class"

/*********************************/
/* Serial/Wifi Command CLASS DEF */
/*********************************/

MyCmd::MyCmd(String cmd_, char cmd_char_, String txt_, uint8_t num_par_, String delim_, void (*DoIt_)(MyCmd *obj))
{
  cmd = cmd_;
  cmd_char = cmd_char_;
  txt = txt_;
  delim = delim_;
  num_par = num_par_;
  if (num_par)
    param = new double[num_par];
  param_txt = "";
  DoIt = DoIt_;
}

void MyCmd::SerialPrint()
{
  my_debug("MyCmd SerialPrint: " + String(cmd));
  if (num_par == 0)
  {
    my_debug("Has TXT parameter: " + txt);
  }
  else
    for (uint8_t i=0; i < num_par; i++)
    {
      my_debug("parameter " + String(i) + ": " + String(param[i]));
    }
}

void MyCmd::ScanToken(String token)
{
  if (num_par == 0)
    param_txt = token; // we only received text
  else
  {
    int pos;
    String buffer;
    for (uint8_t i=0; i < num_par; i++)
    { // interprete as data otherwise
      pos = token.indexOf(delim);
      buffer = token.substring(0, pos);
      param[i] = buffer.toDouble(); // other elements
      token.remove(0, pos + delim.length());
    }
  }
}

/*********************************/
/* Cam/Source Config   CLASS DEF */
/*********************************/

void MyConfig::SerialPrint()
{
  // char num_cmds = ThisDevice->sudo ? num_remote_cmds_private : num_remote_cmds_public;
  // vector<MyCmd> my_vector = priv ? RemoteCmdsPrivate : RemoteCmdsPublic;
  char buffer[100];
  send("# Firmware: " + ThisDevice->firmware + " Rev: " + ThisDevice->revision);
  send("# List all (" + String(num_remote_cmds_public) + ") Remote Commands for this device\n");
  send("# Format is Number:command:'code':\"help text\":#parms");
  vector<MyCmd> my_vector = RemoteCmdsPublic;
  for (uint8_t i=0; i < num_remote_cmds_public; i++)
  {
    if (my_vector[i].DoIt == NULL) send("#"); // indicate that this cmd is not implemented
    sprintf(buffer, "%d:%s:%c:%s:%d\n", i + 1, (char *)my_vector[i].cmd.c_str(), my_vector[i].cmd_char, (char *)my_vector[i].txt.c_str(), (int)my_vector[i].num_par);
    send(buffer);
  }

  if (ThisDevice->sudo)
  {
    send("# List all (" + String(num_remote_cmds_private) + ") Private Commands for this device\n");
    send("# Number : command : 'code' : \"help text\" : number of parameters accepted");
    my_vector = RemoteCmdsPrivate;
    for (uint8_t i=0; i < num_remote_cmds_private; i++)
    {
      if (my_vector[i].DoIt == NULL) send("#"); // indicate that this cmd is not implemented
      sprintf(buffer, "%d:%s:%c:%s:%d\n", i + 1, (char *)my_vector[i].cmd.c_str(), my_vector[i].cmd_char, (char *)my_vector[i].txt.c_str(), (int)my_vector[i].num_par);
      send(buffer);
    }
  }
}

void MyConfig::SendJsonCmdInfo() 
{
  cJSON *command_array = cJSON_CreateArray();
  vector<MyCmd> my_vector = RemoteCmdsPublic;
  for (uint8_t i=0; i < num_remote_cmds_public; i++)
  {
    if (my_vector[i].DoIt){ // ignore NULL functions
      cJSON *cmd_array = cJSON_CreateArray();
      cJSON_AddItemToArray(cmd_array, cJSON_CreateString(my_vector[i].cmd.c_str()));       // command name
      char str[2] = {my_vector[i].cmd_char, '\0'};
      cJSON_AddItemToArray(cmd_array, cJSON_CreateString(str));  // command char
      cJSON_AddItemToArray(cmd_array, cJSON_CreateString(my_vector[i].txt.c_str()));       // help
      cJSON_AddItemToArray(cmd_array, cJSON_CreateNumber(my_vector[i].num_par));    // number of params
      cJSON_AddItemToArray(command_array, cmd_array); // Add the command array to the main JSON array
    }
  }
  if (ThisDevice->sudo)
  {
    my_vector = RemoteCmdsPrivate;
    for (uint8_t i=0; i < num_remote_cmds_private; i++)
    {
      cJSON *cmd_array = cJSON_CreateArray();
      cJSON_AddItemToArray(cmd_array, cJSON_CreateString(my_vector[i].cmd.c_str()));       // command name
      char str[2] = {my_vector[i].cmd_char, '\0'};
      cJSON_AddItemToArray(cmd_array, cJSON_CreateString(str));  // command char
      cJSON_AddItemToArray(cmd_array, cJSON_CreateString(my_vector[i].txt.c_str()));       // help
      cJSON_AddItemToArray(cmd_array, cJSON_CreateNumber(my_vector[i].num_par));    // number of params
      cJSON_AddItemToArray(command_array, cmd_array); // Add the command array to the main JSON array
    }
  }
    // Convert to JSON string
    char *json_string = cJSON_PrintUnformatted(command_array);
    send(json_string);
    free(json_string);
}

void MyConfig::SerialInfo()  
{
    // Convert JSON to string
    char *jsonString = cJSON_PrintUnformatted(cJSON_info);
    send(jsonString);  // Output JSON to Serial
    free(jsonString);
}


bool isBaudRateSupported(int baudRate) {

  return false;     // Baud rate is not supported
}

void DoItWriteMem(MyCmd *obj)
{
  uint8_t addr = (uint8_t)obj->param[0];
  uint8_t data = (uint8_t)obj->param[1];
  if (addr > ASIC_CAM_MAX_ADDR) {
    send("Add out of range");
  }else{
    camera->WriteMem(ASIC_CAM_CMD | addr, data);
    send("Done");
  }
}

void DoItReadMem(MyCmd *obj)
{
  uint8_t addr = (uint8_t)obj->param[0];
  if (addr > ASIC_CAM_MAX_ADDR) {
    send("Add out of range");
  }else{
    camera->ReadMem(ASIC_CAM_CMD | addr);
    send("Data: " + String(camera->rx_data));
  }
}

void DoItSetBaudrate(MyCmd *obj)
{
  uint32_t baudRateToCheck = (uint32_t) obj->param[0];
  //end("Baudrate: " + String(baudRateToCheck));
  for (int i = 0; i < (sizeof(supportedBaudRates) / sizeof(supportedBaudRates[0])); i++) {
    if (baudRateToCheck == supportedBaudRates[i]) {
    send("Baudrate accepted");
    Serial.flush();
    sleep(1);
    Serial.end();
    ThisDevice->baudrate = baudRateToCheck;
    Serial.begin(ThisDevice->baudrate);
    return;
    }
  }
  send("Baudrate invalid");
}

void DoItSetAlarm(MyCmd *obj)
{
  en_log = obj->param[0] ? true : false;
}

void DoItSetTime(MyCmd *obj)
{
  RTCtime.Hours = obj->param[0]; // Set the time.
  RTCtime.Minutes = obj->param[1];
  RTCtime.Seconds = obj->param[2];
  M5.Rtc.SetTime(&RTCtime); // and writes the set time to the real time clock.
}

void DoItSetDate(MyCmd *obj)
{
  RTCDate.Date = obj->param[0];
  RTCDate.Month = obj->param[1];
  RTCDate.Year = obj->param[2]; // Set the date.
  M5.Rtc.SetDate(&RTCDate); // and writes the date to the real time clock.
}

void DoItCmds(MyCmd *obj)
{
  ThisDevice->SerialPrint();
}

void DoItGetJsonCmds(MyCmd *obj)
{
  ThisDevice->SendJsonCmdInfo();
}

void DoItListDir(MyCmd *obj)
{
//  SD.begin(TFCARD_CS_PIN, SPI, 40000000);
//  listDir(SD, "/", 0);
  if(SD2.begin(SD_CONFIG)) {
    SD2.ls(LS_R);
    
  } else{
    Serial.print("SD initialization failed with error code: ");
    Serial.println(SD2.card()->errorCode());
  }
//  SD.end();
}

void DoItReadFile(MyCmd *obj)
{
//  SD.begin(TFCARD_CS_PIN, SPI, 40000000);
//  readFile(SD, (char *)obj->param_txt.c_str());
 // SD.end();
}

void DoItDelFiles(MyCmd *obj)
{
  if (SD2.begin(SD_CONFIG)) {
  if (SD2.remove(obj->param_txt.c_str())) {
    Serial.println("File deleted successfully.");
  } else {
    Serial.println("Failed to delete file.");
  }
  
  }
//  SD.begin(TFCARD_CS_PIN, SPI, 40000000);
//  deleteFile(SD, (char *)obj->param_txt.c_str());
//  SD.end();
}

void DoItReboot(MyCmd *obj)
{
  ESP.restart();
}

void DoItReset(MyCmd *obj)
{
  esp_err_t err = esp_camera_deinit();
//  camera->Reset();
  err = esp_camera_init(&camera_config);
  FEATURE_VPORCH->DoIt();
  FEATURE_SUBST->DoIt();
  FEATURE_CHOPPER_SYNC->DoIt();
  CAMERA_XCLK->DoIt();
//  MEM_PCLK_LB->DoIt();
//  MEM_PCLK_HB->DoIt();
  CAMERA_EXPOSER->DoIt();
  MEM_HSYNC->DoIt();
  FEATURE_VC->DoIt();
}

void DoItDataFormat(MyCmd *obj)
{
  FEATURE_WRITE_INT16->state = obj->param[0] ? true : false;
  FEATURE_WRITE_INT16->DoIt();
  FEATURE_WRITE_INT16->Update();
}

void DoItEnLogger(MyCmd *obj)
{
  en_log = obj->param[0] ? true : false;
}

void DoItZero(MyCmd *obj)
{
  en_zero = obj->param[0] ? true : false;
  en_zero_save = en_zero; // make sure frame is saved on power off.
  if (en_zero)
  {
    lv_btnmatrix_set_btn_ctrl(btnm3, 1, LV_BTNMATRIX_CTRL_CHECKED);
    SCOPE->BackupFrame(SCOPE->offset);
  } else{
    lv_btnmatrix_clear_btn_ctrl(btnm3, 1, LV_BTNMATRIX_CTRL_CHECKED);
  }
}

void DoItChopper(MyCmd *obj)
{
  FEATURE_SUBST->state = obj->param[0] ? true : false;
  FEATURE_SUBST->DoIt();
  FEATURE_SUBST->Update();
}

void DoItSetTexp(MyCmd *obj)
{
    // Apply the value
    CAMERA_EXPOSER->cnt = constrain(obj->param[0], CAMERA_EXPOSER->cnt_min, CAMERA_EXPOSER->cnt_max);
    CAMERA_EXPOSER->DoIt();
    CAMERA_EXPOSER->UpdateSlider();
}

void DoItSetChopCyc(MyCmd *obj)
{
    lv_roller_set_selected(roller_chopper_cyc, constrain(obj->param[0], 0, lv_roller_get_option_cnt(roller_chopper_cyc)), LV_ANIM_OFF);
  lv_event_send(roller_chopper_cyc, LV_EVENT_VALUE_CHANGED, NULL);
}

void DoItEnDebug(MyCmd *obj)
{
  en_debug = obj->param[0] ? true : false;
  // Set the log level of the UART console to a lower level
  if (en_debug)
    esp_log_level_set("*", ESP_LOG_VERBOSE); // ESP_LOG_INFO
  else
    esp_log_level_set("*", ESP_LOG_NONE);
  // gpio_install_isr_service(en_debug);
}

void DoItScreen2bmp(MyCmd *obj) {
  if (obj->param[0] < lv_obj_get_child_cnt(lv_tabview_get_content(tabview))) {
   lv_tabview_set_act(tabview, obj->param[0], LV_ANIM_OFF);
   lv_timer_handler();
  } 
  const uint pad = (4 - (3 * LCD_WIDTH) % 4) % 4; // Padding calculation
  uint filesize = 54 + (3 * LCD_WIDTH + pad) * LCD_HEIGHT;

  unsigned char header[54] = {
      'B', 'M',    // BMP signature
      0, 0, 0, 0,  // File size placeholder
      0, 0, 0, 0,  // Reserved
      54, 0, 0, 0, // Pixel data offset
      40, 0, 0, 0, // DIB header size
      0, 0, 0, 0,  // Image width
      0, 0, 0, 0,  // Image height
      1, 0,        // Planes
      24, 0,       // Bits per pixel
      0, 0, 0, 0,  // Compression
      0, 0, 0, 0,  // Image size
      0, 0, 0, 0,  // X resolution
      0, 0, 0, 0,  // Y resolution
      0, 0, 0, 0,  // Colors used
      0, 0, 0, 0   // Important colors
  };

  // Fill header with correct values
  for (uint i = 0; i < 4; i++) {
    header[2 + i] = (char)((filesize >> (8 * i)) & 255);
    header[18 + i] = (char)((LCD_WIDTH >> (8 * i)) & 255);
    header[22 + i] = (char)((LCD_HEIGHT >> (8 * i)) & 255);
  }

  // Send header
  (MuxSerialToWifi ? client.write(header, 54) : Serial.write(header, 54));

  // Line buffer (width * 3 bytes for RGB + padding)
  unsigned char line_data[LCD_WIDTH * 3 + pad];

  // Initialize padding bytes
  for (int i = (LCD_WIDTH - 1) * 3; i < (LCD_WIDTH * 3 + pad); i++) {
    line_data[i] = 0;
  }

  // Read screen data from bottom to top
  for (int y = LCD_HEIGHT; y > 0; y--) {
    // Read line from screen
    M5.Lcd.readRectRGB(0, y - 1, LCD_WIDTH, 1, line_data);

    // Convert RGB to BGR (swap Red and Blue)
    for (int x = 0; x < LCD_WIDTH; x++) {
      unsigned char r_buff = line_data[x * 3];
      line_data[x * 3] = line_data[x * 3 + 2];
      line_data[x * 3 + 2] = r_buff;
    }

    // Send line data
    (MuxSerialToWifi ? client.write(line_data, (LCD_WIDTH * 3) + pad) : Serial.write(line_data, (LCD_WIDTH * 3) + pad));

    // Acknowledgment handshake
    unsigned long start_time = millis();
    while (millis() - start_time < 1000 && (MuxSerialToWifi ? client.available() : Serial.available()) == 0) {
      delay(10);  // Prevent CPU overload
    }

    if (MuxSerialToWifi ? client.available() : Serial.available()) {
      char ack = MuxSerialToWifi ? client.read() : Serial.read();
      if (ack != 'A') {
        // Receiver didn't acknowledge, abort transmission
        break;
      }
    } else {
      // Timeout: No acknowledgment received
      break;
    }
  }
}

void DoItSetPot(MyCmd *obj)
{
  MyPOT *p_pot;
  uint8_t chan = (uint8_t)obj->param[0];
  switch (chan)
  {
  case IDX_POT_DET:
    p_pot = POT_DET;
    break;
  case IDX_POT_REF:
    p_pot = POT_REF;
    break;
  case IDX_POT_VCO:
    p_pot = POT_VCO;
    break;
  }
  if ((p_pot->Cnt2Volt(p_pot->cnt_min) <= obj->param[1]) && (obj->param[1] <= p_pot->Cnt2Volt(p_pot->cnt_max)))
  {
    p_pot->cnt = p_pot->Volt2Cnt(obj->param[1]);
    lv_slider_set_value(p_pot->slider, p_pot->cnt, LV_ANIM_ON);
    lv_event_send(p_pot->slider, LV_EVENT_VALUE_CHANGED, NULL);
    p_pot->DoIt();
    send("Done! Set POT " + String(chan) + " to " + String(obj->param[1]));
  }
  else
  {
    send("=== Set POT" + String(chan) + " out of bound, cmd ignored ===");
  }
}

void DoItSudo(MyCmd *obj)
{
  SHA256 sha256;
  uint8_t input_hash[SHA256_LENGTH];
  sha256.reset();
  sha256.update(obj->param_txt.c_str(), obj->param_txt.length());
  sha256.finalize(input_hash, sizeof(input_hash));
  if (en_debug)
  {
    for (int i = 0; i < sizeof(input_hash); i++)
    {
      Serial.print(input_hash[i], HEX);
    }
    for (int i = 0; i < sizeof(input_hash); i++)
    {
      Serial.print(sudo_passwd[i], HEX);
    }
  }
  if (memcmp(sudo_passwd, input_hash, sizeof(input_hash)) == 0)
  {
    send("Passwd Accepted");
    ThisDevice->sudo = true;
  }
  else
  {
    send("Access Denied");
    ThisDevice->sudo = false;
  }
}

void DoItSaveCVS(MyCmd *obj){
//  if (SD.begin(TFCARD_CS_PIN, SPI, 40000000)) {
  if (SD2.begin(SD_CONFIG)) {

  uint16_t num_frames = (uint16_t)obj->param[0];
//void readFile(fs::FS &fs, char *path)
  FsFile csv; // = SD2.open("/Images.csv", FILE_WRITE);

  String baseName = "Image";
  String fileName = baseName + ".csv";
  int fileNumber = 0;

  while (SD2.exists(fileName.c_str())) {
    // If the file exists, increment the number to try a new filename
    fileNumber++;
    fileName = baseName + String(fileNumber) + ".csv";
  }

  if (!csv.open(fileName.c_str(), O_WRONLY | O_CREAT | O_TRUNC)) {
    my_debug("open csv file failed");
    return;
  }
  fileName = "save " + fileName + " to SD...";
  show_popup(lv_layer_sys(), fileName.c_str());

  for (uint16_t i = 0; i < num_frames; i++)
  {
    camera_fb_t *fb = SCOPE->FbGet(OR_DEFAULT);
    //SCOPE->ImageProcessing(fb);
    SCOPE->Save(fb, csv);
    esp_camera_fb_return(fb); 
    //usleep(10);
  }
  csv.sync();
  csv.close();
  delete_popup(lv_layer_sys());
  
  }else{
    my_debug("SD card not mounted");
  }
  //SD2.end(); // do not end this -> crashes
  
}

void DoItSaveMJPEG(MyCmd *obj){
  if (SD2.begin(SD_CONFIG)) {

  String baseName = "Video";
  String fileName = baseName + ".mjpg";
  int fileNumber = 0;

  while (SD2.exists(fileName.c_str())) {
    // If the file exists, increment the number to try a new filename
    fileNumber++;
    fileName = baseName + String(fileNumber) + ".mjpg";
  }
  
  FsFile videoFile;
  
  if (!videoFile.open(fileName.c_str(), O_WRONLY | O_CREAT | O_TRUNC)) {
    my_debug("open video MJPEG file failed");
    return;
  }
  fileName = "save " + fileName + " to SD...";
  show_popup(lv_layer_sys(), fileName.c_str());

  long durationMillis = 1000 * obj->param[0];
    SCOPE->graph_idx = SCOPE_TYPE_STREAM;
    camera_fb_t *fb = NULL;
    struct timeval _timestamp;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t *_jpg_buf = NULL;
    char *part_buf[128];

    static int64_t last_frame = 0;
    if (!last_frame)
    {
        last_frame = esp_timer_get_time();
    }

//    while (true)
//    {
  unsigned long startTime = millis();
  while (millis() - startTime < durationMillis) {

  uint8_t orientation;
  if(FEATURE_HMIRROR->state && FEATURE_VMIRROR->state) orientation = OR_ROTATE90_LEFT;
  else if(FEATURE_VMIRROR->state) orientation = OR_VMIRROR;
  else if(FEATURE_HMIRROR->state) orientation = OR_HMIRROR;
  else if(FEATURE_ROTATE90->state) orientation = OR_ROTATE90_RIGHT;
  else orientation = OR_DEFAULT;

        fb = SCOPE->FbGet(orientation);

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
                if (fb->format != PIXFORMAT_JPEG)
                {
                    bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
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
        }
        if (res == ESP_OK)
        {
//            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        if (res == ESP_OK)
        {
//            size_t hlen = snprintf((char *)part_buf, 128, _STREAM_PART, _jpg_buf_len, _timestamp.tv_sec, _timestamp.tv_usec);
//            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if (res == ESP_OK)
        {
//            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
              videoFile.write(_jpg_buf, _jpg_buf_len);
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
            log_e("Save MJPEG failed");
            break;
        }
if(0){
        int64_t fr_end = esp_timer_get_time();
        int64_t frame_time = fr_end - last_frame;
        frame_time /= 1000;
        last_frame = fr_end; // somehow I missed this here!!!
    //    uint32_t avg_frame_time = ra_filter_run(&ra_filter, frame_time);
    }
    }

    SCOPE->graph_idx = SCOPE_TYPE_VIDEO;
//    return res;
  videoFile.sync();
  videoFile.close();
  delete_popup(lv_layer_sys());
  
  }else{
    my_debug("SD card not mounted");
  }
  //SD.end();
}

void DoItStream(MyCmd *obj)
{
  show_popup(lv_layer_sys(), "Streaming mjpeg");

  long durationMillis = 1000 * obj->param[0];
    SCOPE->graph_idx = SCOPE_TYPE_STREAM;
    camera_fb_t *fb = NULL;
    struct timeval _timestamp;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t *_jpg_buf = NULL;
    char *part_buf[128];

    static int64_t last_frame = 0;
    if (!last_frame)
    {
        last_frame = esp_timer_get_time();
    }
  unsigned long startTime = millis();
  while (millis() - startTime < durationMillis) {

  uint8_t orientation;
  if(FEATURE_HMIRROR->state && FEATURE_VMIRROR->state) orientation = OR_ROTATE90_LEFT;
  else if(FEATURE_VMIRROR->state) orientation = OR_VMIRROR;
  else if(FEATURE_HMIRROR->state) orientation = OR_HMIRROR;
  else if(FEATURE_ROTATE90->state) orientation = OR_ROTATE90_RIGHT;
  else orientation = OR_DEFAULT;

        fb = SCOPE->FbGet(orientation);

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
                if (fb->format != PIXFORMAT_JPEG)
                {
                    bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
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
        }
        if (res == ESP_OK)
        {
//            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        if (res == ESP_OK)
        {
//            size_t hlen = snprintf((char *)part_buf, 128, _STREAM_PART, _jpg_buf_len, _timestamp.tv_sec, _timestamp.tv_usec);
//            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if (res == ESP_OK)
        {
//            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
//              videoFile.write(_jpg_buf, _jpg_buf_len);
           MuxSerialToWifi ? client.write(_jpg_buf, _jpg_buf_len) : Serial.write(_jpg_buf, _jpg_buf_len);  

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
            log_e("Save MJPEG failed");
            break;
        }
if(0){
        int64_t fr_end = esp_timer_get_time();
        int64_t frame_time = fr_end - last_frame;
        frame_time /= 1000;
        last_frame = fr_end; // somehow I missed this here!!!
    //    uint32_t avg_frame_time = ra_filter_run(&ra_filter, frame_time);
    }
    }

    SCOPE->graph_idx = SCOPE_TYPE_VIDEO;
//    return res;
  delete_popup(lv_layer_sys());
}

void DoItGetImages(MyCmd *obj)
{
  uint16_t num_frames = (uint16_t)obj->param[0];
  show_popup(lv_layer_sys(), "GetImages"); 
  for (uint16_t i = 0; i < num_frames; i++)
  {
    camera_fb_t *fb = SCOPE->FbGet(OR_DEFAULT);
//    SCOPE->ImageProcessing(fb);
    SCOPE->Send(fb);
    esp_camera_fb_return(fb);
    unsigned long start_time = millis();
    if(MuxSerialToWifi){
      while (millis() - start_time < 1000 && client.available() == 0)
      {
        // Wait for an acknowledgement signal from the receiver for up to 1 second
      }
      if (client.available() > 0)
        {
          char ack = client.read();
          if (ack == 'A')
          {
            // We are on track
          }
        }
        else
        {
          // Receiver did not acknowledge the data, stop transmitting
          break;
        }
    } else{
      while (millis() - start_time < 1000 && Serial.available() == 0)
      {
        // Wait for an acknowledgement signal from the receiver for up to 1 second
      }
      if (Serial.available() > 0)
      {
        char ack = Serial.read();
        if (ack == 'A')
        {
          // We are on track
        }
      }
      else
      {
        // Receiver did not acknowledge the data, stop transmitting
        break;
      }
    }
  }
  delete_popup(lv_layer_sys());
}

void DoItStatus(MyCmd *obj)
{
  send("TicWave GmbH Camera Status");
  send("Note: Enable Performance Monitor to get full status\n");
  uint16_t num_rows = lv_table_get_row_cnt(table);
//  uint16_t num_cols = lv_table_get_col_cnt(table);
#if CONFIG_MONITOR_VERBOSE
  MYM5_DATE->DoIt();
  MYM5_DATE->UpdateTable();
#endif
  for (uint16_t row = 0; row < num_rows; row++) {
    String cell_text = lv_table_get_cell_value(table, row, 0);
    String cell_value = lv_table_get_cell_value(table, row, 1);
    std::replace(cell_text.begin(), cell_text.end(), '\n', ' ');
    std::replace(cell_value.begin(), cell_value.end(), '\n', ' ');
    send(cell_text + ": " + cell_value + "\n");
  }
}

void DoItInfo(MyCmd *obj)
{
  ThisDevice->SerialInfo();
}

MyConfig::MyConfig(String firmware_, String revision_, String wifi_ssid_, String wifi_passwd_)
{
  firmware = firmware_;
  revision = revision_;
  wifi_ssid = wifi_ssid_;
  wifi_passwd = wifi_passwd_;
  sudo = false;
  num_remote_cmds_public = 0;
  num_remote_cmds_private = 0;
  baudrate = DEFAULT_SERIAL_BAUDRATE;

  cJSON_info = cJSON_CreateObject();
  cJSON_AddStringToObject(cJSON_info, "model", MODEL);
  cJSON_AddStringToObject(cJSON_info, "firmware", FIRMWARE);
  cJSON_AddStringToObject(cJSON_info, "revision", REVISION);
  cJSON_AddNumberToObject(cJSON_info, "chip ID", (double) chip_id);
  cJSON_AddStringToObject(cJSON_info, "SSID", wifi_ssid.c_str());
  cJSON_AddStringToObject(cJSON_info, "passwd", wifi_passwd.c_str());
  cJSON_AddNumberToObject(cJSON_info, "baudrate", (double) baudrate);
  cJSON *resolution = cJSON_CreateObject();
  cJSON_AddNumberToObject(resolution, "width", NUMX);
  cJSON_AddNumberToObject(resolution, "height", NUMY);
  cJSON_AddItemToObject(cJSON_info, "resolution", resolution);
  
  // Public Commands, available to the customers
  // do not use b (fixed useage in CAMctrl.py)

// ToDo: 
// emplace_back instead of push_back
// 

  char letter = 'a';  // Start first cmd at 'a'
  RemoteCmdsPublic.push_back(MyCmd("GetCmds", letter++, "Get JSON command info", 0, " ", &DoItGetJsonCmds));
  //RemoteCmdsPublic.push_back(MyCmd("Update", 'a', "Update config file with known commands from device", 0, " ", &DoItGetCmds));
  RemoteCmdsPublic.push_back(MyCmd("GetStatus", letter++, "Get Camera Status", 1, " ", &DoItStatus));
  RemoteCmdsPublic.push_back(MyCmd("GetInfo", letter++, "Get Camera Info", 1, " ", &DoItInfo));
  RemoteCmdsPublic.push_back(MyCmd("GetMAT", letter++, "Save .mat file for N=frames", 1, " ", &DoItGetImages));
  RemoteCmdsPublic.push_back(MyCmd("Stream", letter++, "Stream mjpeg video for N=seconds", 1, " ", &DoItStream));
  RemoteCmdsPublic.push_back(MyCmd("SaveCSV", letter++, "Export .csv data to SD, N=frames", 1, " ", &DoItSaveCVS));
  RemoteCmdsPublic.push_back(MyCmd("SaveMJPG", letter++, "Export .mjpeg video to SD, N=seconds", 1, " ", &DoItSaveMJPEG));
  RemoteCmdsPublic.push_back(MyCmd("SetBaud", letter++, "Set N=Baudrate", 1, " ", &DoItSetBaudrate)); // do not change s here, it is fixed encoded in CAMctrl.py
  RemoteCmdsPublic.push_back(MyCmd("Reboot", letter++, "Reboot camera", 0, " ", &DoItReboot));
  RemoteCmdsPublic.push_back(MyCmd("Reset", letter++, "Reset camera chip", 0, " ", &DoItReset));
  RemoteCmdsPublic.push_back(MyCmd("DataFormat", letter++, "Set data format N=1 (int16), N=0 default (int32)", 1, " ", &DoItDataFormat));
  RemoteCmdsPublic.push_back(MyCmd("Zero", letter++, "Zero Camera N=state", 1, " ", &DoItZero));
  RemoteCmdsPublic.push_back(MyCmd("EnChopper", letter++, "Enable chopper/CW N=state", 1, " ", &DoItChopper));
  RemoteCmdsPublic.push_back(MyCmd("SetTexp", letter++, "Set exposer time N in us", 1, " ", &DoItSetTexp));
  RemoteCmdsPublic.push_back(MyCmd("SetChopCyc", letter++, "Set chopper cyc N=cyc", 1, " ", &DoItSetChopCyc));
  RemoteCmdsPublic.push_back(MyCmd("EnSudo", letter++, "Enable sudo mode (req. passwd)", 0, " ", &DoItSudo));

  // Private Commands, hidden to the customers
  RemoteCmdsPrivate.push_back(MyCmd("SetPot", letter++, "Set Pot with N1=(0, 1, 2 for Vref,Det,VCO) N2=Voltage", 2, " ", &DoItSetPot));
  RemoteCmdsPrivate.push_back(MyCmd("ReadMem", letter++, "Read Memory, N=address", 1, " ", &DoItReadMem));
  RemoteCmdsPrivate.push_back(MyCmd("WriteMem", letter++, "Write Memory, N1=address N2=data", 2, " ", &DoItWriteMem));
  RemoteCmdsPrivate.push_back(MyCmd("SetTime", letter++, "Set Time h m s", 3, " ", &DoItSetTime));
  RemoteCmdsPrivate.push_back(MyCmd("SetDate", letter++, "Set Date d m y", 3, " ", &DoItSetDate));
  RemoteCmdsPrivate.push_back(MyCmd("GetState", letter++, "Get State", 1, " ", NULL));
  RemoteCmdsPrivate.push_back(MyCmd("EnDebug", letter++, "Enable Debug", 1, " ", &DoItEnDebug));
  RemoteCmdsPrivate.push_back(MyCmd("EnLogger", letter++, "Enable Data Logger", 1, " ", &DoItEnLogger));
  RemoteCmdsPrivate.push_back(MyCmd("ListDir", letter++, "List Dir for N lines", 1, " ", &DoItListDir));
  RemoteCmdsPrivate.push_back(MyCmd("ReadFile", letter++, "Read File from SD", 0, " ", &DoItReadFile));
  RemoteCmdsPrivate.push_back(MyCmd("DelFile", letter++, "Delete File from SD", 0, " ", &DoItDelFiles));
  RemoteCmdsPrivate.push_back(MyCmd("GetScreen", letter++, "Get Screen Shot, N=tabview", 1, " ", &DoItScreen2bmp));

  num_remote_cmds_public = RemoteCmdsPublic.size();
  num_remote_cmds_private = RemoteCmdsPrivate.size();
}

/*********************************/
/* Serial/Wifi Scanner CLASS DEF */
/*********************************/

MyScan::MyScan(String delim_)
{
  cmd = 0;
  token = "";
  delim = delim_;
}

void MyScan::SplitLine(String line)
{
  int pos;
  pos = line.indexOf(delim);
  token = line.substring(0, pos);
  cmd = token.charAt(0);                                           // first element
                                                                   //  line.remove(0, pos + deliMyScanm.length());
  token = line.substring(pos + delim.length(), line.length() - 1); // us -1 to remove \n from sting as well
}

void MyScan::SerialPrint()
{
  my_debug("MyScan SerialPrint cmd: " + String(cmd) + " token: " + String(token));
}

void MyScan::DoIt()
{
  bool found = false;
  //SCOPE->graph_idx = SCOPE_TYPE_IDLE; // make sure there is nothing running in the background
  my_debug("MyScan DoIt cmd: " + String(cmd));
  my_debug("Scan through public commands: " + String(ThisDevice->num_remote_cmds_public));
  for (uint8_t i=0; i < ThisDevice->num_remote_cmds_public; i++)
  {
    if (cmd == ThisDevice->RemoteCmdsPublic[i].cmd_char)
    {
      ThisDevice->RemoteCmdsPublic[i].ScanToken(token);
      my_debug("Found: " + ThisDevice->RemoteCmdsPublic[i].cmd + " Help: " + ThisDevice->RemoteCmdsPublic[i].txt);
      ThisDevice->RemoteCmdsPublic[i].SerialPrint();
      ThisDevice->RemoteCmdsPublic[i].DoIt(&ThisDevice->RemoteCmdsPublic[i]); // Execute the command, pass pointer to command here
      found = true;
      break;
    }
  }
  if (ThisDevice->sudo)
  {
    my_debug("Scan through private commands: " + String(ThisDevice->num_remote_cmds_private));
    for (uint8_t i=0; i < ThisDevice->num_remote_cmds_private; i++)
    {
      if (cmd == ThisDevice->RemoteCmdsPrivate[i].cmd_char)
      {
        ThisDevice->RemoteCmdsPrivate[i].ScanToken(token);
        my_debug("Found: " + ThisDevice->RemoteCmdsPrivate[i].cmd + " Help: " + ThisDevice->RemoteCmdsPrivate[i].txt);
        ThisDevice->RemoteCmdsPrivate[i].SerialPrint();
        ThisDevice->RemoteCmdsPrivate[i].DoIt(&ThisDevice->RemoteCmdsPrivate[i]); // Execute the command, pass pointer to command here
        found = true;
        break;
      }
    }
  }
  if (!found)
  {
    send("command unknown");
  }
}

/********************/
/* DEVICE CLASS DEF */
/********************/

MyDevice::~MyDevice()
{
}

MyDevice::MyDevice(String txt_, String fmt_, uint16_t cnt_, uint16_t cnt_min_, uint16_t cnt_max_, float norm_)
{
  txt = txt_;
  fmt = fmt_;
  cnt = cnt_;
  cnt2 = 0; // slider left value
  cnt_min = cnt_min_;
  cnt_max = cnt_max_;
  norm = norm_;
  slider_mode = LV_SLIDER_MODE_NORMAL;
}

void MyDevice::SliderCB(lv_event_t *event)
{
  lv_event_code_t code = lv_event_get_code(event);
  lv_obj_t *obj = lv_event_get_target(event);
  MyDevice *myClass = (MyDevice *)lv_event_get_user_data(event);
  if (code == LV_EVENT_RELEASED)
  {
    //  my_debug("value: " + String(lv_slider_get_value(obj) << DAC_BIT_SHIFT));
    //  Serial.printf("user_data: %X\n", myClass);
    myClass->cnt = (uint16_t)lv_slider_get_value(obj); // slider can not handle 16 bit, use reduced resolution for slider

    if(myClass->slider_mode == LV_SLIDER_MODE_RANGE){
    myClass->cnt2 = (uint16_t)lv_slider_get_left_value(obj); // slider can not handle 16 bit, use reduced resolution for slider
    } 
    lv_table_set_cell_value_fmt(myClass->table, myClass->tab_idx, 1, (const char *)myClass->fmt.c_str(), myClass->Cnt2Volt(myClass->cnt));
    myClass->DoIt();
  }
  else if (code == LV_EVENT_VALUE_CHANGED)
  {
    if(myClass->slider_mode == LV_SLIDER_MODE_RANGE) lv_label_set_text_fmt(myClass->slider_label, (const char *)myClass->fmt.c_str(), myClass->Cnt2Volt((uint16_t)lv_slider_get_left_value(obj)), myClass->Cnt2Volt((uint16_t)lv_slider_get_value(obj)));
    else lv_label_set_text_fmt(myClass->slider_label, (const char *)myClass->fmt.c_str(), myClass->Cnt2Volt((uint16_t)lv_slider_get_value(obj)));
  }
}

void MyDevice::DoIt()
{
  my_debug("DoIt: - no action yet - " + String(cnt) + " " + String(Cnt2Volt(cnt)));
}

void MyDevice::SliderCreate(int posy, lv_slider_mode_t mode, lv_obj_t *obj)
{
  slider_mode = mode;
  char buf[40];
  slider = lv_slider_create(obj);
  lv_obj_add_event_cb(slider, this->SliderCB, LV_EVENT_ALL, this);
  // Serial.printf("This: %X\n", this);
  
  lv_obj_set_width(slider, 130 * 1.2);
  lv_obj_set_pos(slider, 60, posy);
  lv_slider_set_range(slider, cnt_min, cnt_max); // 65536
  
  lv_slider_set_value(slider, cnt, LV_ANIM_OFF);
  /* Create a label next to the slider */
  slider_label = lv_label_create(obj);
  lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_LEFT_MID, -30, 0);
  info_txt = lv_label_create(obj);

  if(mode == LV_SLIDER_MODE_RANGE){
  lv_slider_set_mode(slider, LV_SLIDER_MODE_RANGE);
  lv_slider_set_left_value(slider, 0, LV_ANIM_OFF);
  lv_label_set_text_fmt(slider_label, (const char *)fmt.c_str(), Cnt2Volt(cnt2), Cnt2Volt(cnt));
  sprintf(buf, "%s (%1.0f-%1.0f)", (const char *)txt.c_str(), Cnt2Volt(cnt_min), Cnt2Volt(cnt_max));
  }else{
    lv_label_set_text_fmt(slider_label, (const char *)fmt.c_str(), Cnt2Volt(cnt));
    sprintf(buf, "%s %.2f (%.1f-%.1f)", (const char *)txt.c_str(), Cnt2Volt(cnt), Cnt2Volt(cnt_min), Cnt2Volt(cnt_max));
  }
  lv_label_set_text(info_txt, buf);
  lv_obj_align_to(info_txt, slider, LV_ALIGN_OUT_TOP_MID, 0, -10);
  lv_label_set_text_fmt(slider_label, (const char *)fmt.c_str(), Cnt2Volt(cnt));
}

void MyDevice::TableCreate(int tab_idx_, lv_obj_t *table_)
{
  table = table_;
  tab_idx = tab_idx_;
  lv_table_set_cell_value(table, tab_idx, 0, (const char *)txt.c_str());
  lv_table_set_cell_value_fmt(table, tab_idx, 1, (const char *)fmt.c_str(), Cnt2Volt(cnt));
}

float MyDevice::Cnt2Volt(uint16_t my_cnt)
{
  return (norm * my_cnt);
}

uint16_t MyDevice::Volt2Cnt(float volt)
{
  return ((uint16_t)(volt / norm + 0.5));
}

void MyDevice::UpdateTable()
{
  lv_table_set_cell_value_fmt(table, tab_idx, 1, (const char *)fmt.c_str(), Cnt2Volt(cnt));
  // lv_slider_set_value(slider, cnt , LV_ANIM_ON);
  // lv_event_send(slider, LV_EVENT_VALUE_CHANGED, NULL);
}

void MyDevice::UpdateSlider()
{
  lv_slider_set_value(slider, cnt , LV_ANIM_ON);
  lv_event_send(slider, LV_EVENT_VALUE_CHANGED, NULL);
}

void MyDevice::HideSlider()
{
  lv_obj_add_flag(slider, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(slider_label, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(info_txt, LV_OBJ_FLAG_HIDDEN);
}

void MyDevice::UnHideSlider()
{
  lv_obj_clear_flag(slider, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(slider_label, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(info_txt, LV_OBJ_FLAG_HIDDEN);
}

void MyDevice::SerialPrint()
{
  my_debug("Name: " + String(txt) + " " + String(fmt));
  my_debug("cnt: " + String(cnt) + " " + String(Cnt2Volt(cnt)));
  my_debug("cnt_min: " + String(cnt_min) + " " + String(Cnt2Volt(cnt_min)));
  my_debug("cnt_max: " + String(cnt_max) + " " + String(Cnt2Volt(cnt_max)));
  my_debug("norm: " + String(norm));
  // my_debug("dac_chan: " + String(dac_chan));
}

/*****************/
/* DAC CLASS DEF */
/*****************/

MyDAC::MyDAC(uint8_t dac_chan_, String txt_, String fmt_, uint16_t cnt_, uint16_t cnt_min_, uint16_t cnt_max_, float norm_) : MyDevice(txt_, fmt_, cnt_, cnt_min_, cnt_max_, norm_)
{
  dac_chan = dac_chan_;
  DoIt(); // write the default
}

MyDAC::~MyDAC()
{
}

void MyDAC::DoIt()
{
  my_debug("DAC DoIt SetChannel: DAC" + String(dac_chan) + " " + String(cnt) + " " + String(Cnt2Volt(cnt)));
  /*
  DAC_AD5686.SetChannel(dac_chan, cnt);
*/
}

/*****************/
/* POT CLASS DEF */
/*****************/

MyPOT::MyPOT(MCP466_DigitalPot *pot_, uint8_t pot_chan_, String txt_, String fmt_, uint16_t cnt_, uint16_t cnt_min_, uint16_t cnt_max_, float norm_) : MyDevice(txt_, fmt_, cnt_, cnt_min_, cnt_max_, norm_)
{
  pot_chan = pot_chan_;
  pot = pot_;
  DoIt(); // write the volatile default
  // pot->mcpWrite(pot_chan+2, 255 - cnt);  // write the non-volatile default
}

MyPOT::~MyPOT()
{
}

void MyPOT::DoIt()
{
  my_debug("POT DoIt mcpWrite: POT" + String(pot_chan) + " " + String(cnt) + " " + String(Cnt2Volt(cnt)));
  pot->mcpWrite(pot_chan, 255 - cnt);
}

/*****************/
/* PWM CLASS DEF */
/*****************/

MyPWM::MyPWM(uint8_t pwm_pin_, uint8_t pwm_chan_, uint16_t pwm_freq_, String txt_, String fmt_, uint16_t cnt_, uint16_t cnt_min_, uint16_t cnt_max_, float norm_) : MyDevice(txt_, fmt_, cnt_, cnt_min_, cnt_max_, norm_)
{
  pwm_pin = pwm_pin_;
  pwm_chan = pwm_chan_;
  pwm_freq = pwm_freq_;
  /*
  ledcSetup(pwm_chan, pwm_freq, PWM_RES_BITS);
  AttachPin();
  */
  my_debug("PWM init PIN: " + String(pwm_pin) + " CHAN: " + String(pwm_chan) + " FREQ: " + String(pwm_freq));
}

MyPWM::~MyPWM()
{
}

void MyPWM::DoIt()
{
  my_debug("PWM DoIt ledcWrite: PWM" + String(pwm_chan) + " " + String(cnt) + " " + String(Cnt2Volt(cnt)));
  /*
  ledcWrite(pwm_chan, cnt);
  */
}

void MyPWM::DetachPin()
{
  /*
  ledcDetachPin(pwm_pin);
  */
}

void MyPWM::AttachPin()
{
  /*
  ledcAttachPin(pwm_pin, pwm_chan);
  ledcWrite(pwm_chan, PWM_RES/2);
  */
}

/*********************/
/* CHOPPER CLASS DEF */
/*********************/

MyCHOP::MyCHOP(uint8_t pwm_pin_, uint8_t pwm_chan_, uint16_t pwm_freq_, String txt_, String fmt_, uint16_t cnt_, uint16_t cnt_min_, uint16_t cnt_max_, float norm_) : MyPWM(pwm_pin_, pwm_chan_, pwm_freq_, txt_, fmt_, cnt_, cnt_min_, cnt_max_, norm_)
{
}

MyCHOP::~MyCHOP()
{
}

void MyCHOP::DoIt()
{
  my_debug("MyCHOP DoIt ledcChangeFrequency: PWM" + String(pwm_chan) + " " + String(cnt) + " " + String(Cnt2Volt(cnt)));
  pwm_freq = cnt;
  /*
    ledcChangeFrequency(pwm_chan, cnt, PWM_RES_BITS);
  */
}

/*****************/
/* LDO CLASS DEF */
/*****************/

MyLDO::MyLDO(uint8_t ldo_chan_, String txt_, String fmt_, uint16_t cnt_, uint16_t cnt_min_, uint16_t cnt_max_, float norm_) : MyDevice(txt_, fmt_, cnt_, cnt_min_, cnt_max_, norm_)
{
  ldo_chan = ldo_chan_;
}

MyLDO::~MyLDO()
{
}

void MyLDO::DoIt()
{
  my_debug("LDO DoIt SetLDOVoltage: LDO" + String(ldo_chan) + " " + String(cnt) + " " + String(Cnt2Volt(cnt)));
  M5.Axp.SetLDOVoltage(ldo_chan, norm * cnt * 1000); // set in mV
}

/*****************/
/* ADC CLASS DEF */
/*****************/

MyADC::MyADC(uint16_t adc_chan_, uint16_t tx_data_, String txt_, String fmt_, uint16_t cnt_max_, float norm_) : MyDevice(txt_, fmt_, 0, 0, cnt_max_, norm_)
{
  tx_data = tx_data_ | adc_chan_;
  adc_chan = adc_chan_;
  rx_data = 0;
  // cnt=0;
  // cnt_min=0;
}

MyADC::~MyADC()
{
}

void MyADC::BeginTransaction()
{
  SPI.beginTransaction(SPISettings(ADC_SPI_Clock, MSBFIRST, SPI_MODE2));
}

void MyADC::EndTransaction()
{
  SPI.endTransaction();
}

void MyADC::DoItFast()
{
  // first round to select right channel
  /*
   digitalWrite(GPIO_ADC_SS, LOW);
   SPI.transferBits(tx_data << 4, &rx_data, 16); // tx data needs left shift by 4 (is a 16 bit shift reg. but ony 12 bit dac)
   digitalWrite(GPIO_ADC_SS, HIGH);
   // now get the channel
   digitalWrite(GPIO_ADC_SS, LOW);
   SPI.transferBits(tx_data << 4, &rx_data, 16); // tx data needs left shift by 4 (is a 16 bit shift reg. but ony 12 bit dac)
   digitalWrite(GPIO_ADC_SS, HIGH);
   SPI.endTransaction();
   cnt = (uint16_t) rx_data & 0xFFF; // lower 12 bits are data
 */
  // my_debug("ADC DoIt transferBits: ADC" + String(tx_data) + " " + String(cnt) + " " + String(Cnt2Volt(cnt)));  // move this to seperate function
}

void MyADC::DoIt()
{
  /*
    SPI.beginTransaction(SPISettings(ADC_SPI_Clock, MSBFIRST, SPI_MODE2));
    // first round to select right channel
    digitalWrite(GPIO_ADC_SS, LOW);
    SPI.transferBits(tx_data << 4, &rx_data, 16); // tx data needs left shift by 4 (is a 16 bit shift reg. but ony 12 bit dac)
    digitalWrite(GPIO_ADC_SS, HIGH);
    // now get the channel
    digitalWrite(GPIO_ADC_SS, LOW);
    SPI.transferBits(tx_data << 4, &rx_data, 16); // tx data needs left shift by 4 (is a 16 bit shift reg. but ony 12 bit dac)
    digitalWrite(GPIO_ADC_SS, HIGH);
    SPI.endTransaction();
    cnt = (uint16_t) rx_data & 0xFFF; // lower 12 bits are data
  */
  // my_debug("ADC DoIt transferBits: ADC" + String(tx_data) + " " + String(cnt) + " " + String(Cnt2Volt(cnt)));  // move this to seperate function
}

/*****************/
/* ADCi2c CLASS DEF */
/*****************/

MyADCi2c::MyADCi2c(uint8_t i2cAddress, String txt_, String fmt_, uint16_t cnt_max_, float norm_) : MyDevice(txt_, fmt_, 0, 0, cnt_max_, norm_)
{
  my_ads = new ADS1100;
  my_ads->setWireInterface(&Wire1);
  my_ads->getAddr_ADS1100(i2cAddress); // 0x48, 1001 000 (ADDR = GND)

  // The ADC gain (PGA), Device operating mode, Data rate
  // can be changed via the following functions
  my_ads->setGain(GAIN_ONE); // 1x gain(default)
  // my_ads->setGain(GAIN_TWO);       // 2x gain
  //  ads.setGain(GAIN_FOUR);      // 4x gain
  //  ads.setGain(GAIN_EIGHT);     // 8x gain

  // ads.setMode(MODE_CONTIN);       // Continuous conversion mode (default)
  my_ads->setMode(MODE_SINGLE); // Single-conversion mode

  my_ads->setRate(RATE_8); // 8SPS (default)
  // ads.setRate(RATE_16);        // 16SPS
  // ads.setRate(RATE_32);        // 32SPS
  // ads.setRate(RATE_128);       // 128SPS

  my_ads->setOSMode(OSMODE_SINGLE); // Set to start a single-conversion
}

MyADCi2c::~MyADCi2c()
{
}

void MyADCi2c::DoIt()
{
  //  cnt = my_ads->Measure_Differential() + (ADC_RANGE_ADS1100 >> 1);
  //  cnt = static_cast<uint16_t>(my_ads->Measure_Differential() & INT16_MAX);
  cnt = my_ads->Measure_Singleended();
  my_debug("ADCi2c DoIt: " + String(cnt));
}

/***********************/
/* ADC ESP32 CLASS DEF */
/***********************/

MyADCESP::MyADCESP(uint8_t gpio_adc_, String txt_, String fmt_, uint16_t cnt_max_, float norm_) : MyDevice(txt_, fmt_, 0, 0, cnt_max_, norm_)
{
  gpio_adc = gpio_adc_;
  pinMode(gpio_adc, INPUT);
}

void MyADCESP::DoIt()
{
  cnt = analogRead(gpio_adc);
  my_debug("ADC ESP32 DoIt analogRead: GPIO" + String(gpio_adc) + " " + String(cnt) + " " + String(Cnt2Volt(cnt)));
}

/*****************/
/* NTC CLASS DEF */
/*****************/

MyNTC::MyNTC(uint8_t gpio_adc_, String txt_, String fmt_, uint16_t cnt_max_, float norm_) : MyDevice(txt_, fmt_, 0, 0, cnt_max_, norm_)
{
  gpio_adc = gpio_adc_;
}

void MyNTC::DoIt()
{
  cnt = analogRead(gpio_adc);
  my_debug("NTC DoIt: GPIO" + String(gpio_adc) + " " + String(cnt) + " " + String(Cnt2Volt(cnt)) + " °C");
}

float MyNTC::Cnt2Volt(uint16_t my_cnt)
{                                                      // use virtual function to do the scaling
                                                       // using the B-parameter equation: 1/T = 1/T0 + ln(R/R0)/B
                                                       // T0 = 298.15K (25° room temp)
                                                       // R0 = 10k (resistance at room temp)
                                                       // R (measured)
                                                       // T (corresponding temp)
                                                       //  float VRT = 0.0 + (3.3 / ADC_RANGE_AD7924) * my_cnt; //read divider voltage, use 3.1 (3.3V -2*0.1V) because of limited range of ADC
  float VRT = 0.1 + (3.1 / ADC_RANGE_AD7924) * my_cnt; // read divider voltage, use 3.1 (3.3V -2*0.1V) because of limited range of ADC
  float I = (3.3 - VRT) / R_NTC_pull;                  // Current through pullup
  float RT = VRT / I;                                  // Resistance of RT
  float ln = log(RT / R_NTC) / B_NTC + 1 / T0_NTC;
  float TX = 1 / ln;
  TX = TX - 273.15; // Convert K to Celsius
  //  float TX = 1/(1/T0_NTC+log(RT/R_NTC)/B_NTC)- 273.15; // all in one eqn
  return (TX); // add +8°C correction (probably due to missing stabelizing cap)
}

/**************************/
/*  TEMP Sensor CLASS DEF */
/**************************/

MyTEMP::MyTEMP(String txt_, String fmt_, float norm_) : MyDevice(txt_, fmt_, 0, 0, ADC_RANGE_ADS1100-1, norm_)
{
}

void MyTEMP::DoIt()
{
  ADC_TEMP->DoIt();
  cnt = ADC_TEMP->cnt;
  my_debug("TEMP DoIt: " + String(cnt) + " " + String(Cnt2Volt(cnt)) + " °C");
}

float MyTEMP::Cnt2Volt(uint16_t my_cnt)
{                                                                     // use virtual function to do the scaling
  float reading_mV = cnt * norm * 1000;                               // in mV
  return (TEMP_Tcal_degC + (TEMP_Vcal_mV - reading_mV) / TEMP_coeff); // assume linear fit (dreisatz)
}

/******************/
/* MYM5 CLASS DEF */
/******************/

MyM5::MyM5(uint8_t dev_idx_, String txt_, String fmt_)
{
  dev_idx = dev_idx_;
  txt = txt_;
  fmt = fmt_;
  tab_idx = 0;
  table = NULL;
  symbol = "";
  voltage = 0;
  current = 0;
  power = 0;
  //  level = 0;
  percentage = 0;
}

void MyM5::DoIt()
{
  switch (dev_idx)
  {
  case MYM5_IDX_BAT:
  { /* Battery Status Indicater */
    voltage = M5.Axp.GetBatVoltage();
    current = M5.Axp.GetBatCurrent();
    power = M5.Axp.GetBatPower();
    //        level = M5.Axp.GetBatteryLevel();
    percentage = (voltage < 3.2) ? 0 : (voltage - 3.2) * 100;
    if (percentage > 80)
    {
      symbol = LV_SYMBOL_BATTERY_FULL;
    }
    else if (percentage > 60)
    {
      symbol = LV_SYMBOL_BATTERY_3;
    }
    else if (percentage > 40)
    {
      symbol = LV_SYMBOL_BATTERY_2;
    }
    else if (percentage > 20)
    {
      symbol = LV_SYMBOL_BATTERY_1;
    }
    else
    {
      symbol = LV_SYMBOL_BATTERY_EMPTY;
    }
  }
  break;
  case MYM5_IDX_WIFI:
  {
  //  if (WiFi.status() == WL_CONNECTED) {
    // Check the current Wi-Fi mode
    int wifiMode = WiFi.getMode();

    if (wifiMode == WIFI_OFF) {
      symbol = "Wi-Fi off";
    } else if (wifiMode == WIFI_STA) {
      symbol = "Wi-Fi STA " + String(WIFI_SSID) + ": " + WiFi.localIP().toString();
    } else if (wifiMode == WIFI_AP) {
      symbol = "Wi-Fi AP " + String(WIFI_SSID_AP) + ": " + WiFi.softAPIP().toString();
    } else if (wifiMode == WIFI_AP_STA) {
      symbol = "Wi-Fi AP-STA " + WiFi.softAPIP().toString() + "," + WiFi.localIP().toString();
    }
    //      WiFi.reconnect();
  }
  break;
  case MYM5_IDX_DATE:
  {
    //char date[30];
    M5.Rtc.GetTime(&RTCtime); //Gets the time in the real-time clock
    symbol = String(RTCDate.Year) + "-" +
              String(RTCDate.Month) + "-" +
              String(RTCDate.Date) + " " +
              String(RTCtime.Hours) + ":" +
              String(RTCtime.Minutes) + ":" +
              String(RTCtime.Seconds);
   // lv_label_set_text_fmt(status_bar_top_label, "%d/%02d/%02d %02d:%02d:%02d", RTCDate.Year,RTCDate.Month,RTCDate.Date,RTCtime.Hours,RTCtime.Minutes,RTCtime.Seconds);
  }
  break;
  case MYM5_IDX_VBUS:
  {
    if (M5.Axp.isVBUS())
    {
      symbol = LV_SYMBOL_USB;
    }
    else
    {
      symbol = "";
    }
    voltage = M5.Axp.GetVBusVoltage();
    current = M5.Axp.GetVBusCurrent();
  }
  break;
  case MYM5_IDX_AC:
  {

    // this is not desired without battary
    //        if(M5.Axp.isACIN()){symbol = LV_SYMBOL_USB; M5.Axp.SetLcdVoltage(3300);}
    //        else{symbol = ""; M5.Axp.SetLcdVoltage(2700);}

    voltage = M5.Axp.GetVinVoltage();
    current = M5.Axp.GetVinCurrent();
  }
  break;
  case MYM5_IDX_IPS:
  {
    voltage = M5.Axp.GetAPSVoltage();
  }
  break;
  case MYM5_IDX_CHARGE:
  { /* Battery Charging Indicater */
    if (M5.Axp.isCharging())
    {
      symbol = LV_SYMBOL_CHARGE;
    }
    else
    {
      symbol = "";
    }
    current = M5.Axp.GetBatChargeCurrent();
    power = M5.Axp.GetBatPower();
  }
  break;
  case MYM5_IDX_TEMP_APX192:
  {
    voltage = M5.Axp.GetTempInAXP192();
  }
  break;
  }
  //  my_debug("MYM5 DoIt DEV: " + String(dev_idx) + " " + String(voltage) + " " + String(current)  + " " + String(power) + " " + String(level) + " " + String(percentage));
  my_debug("MYM5 DoIt DEV: " + String(dev_idx) + " " + String(voltage) + " " + String(current) + " " + String(power) + " " + String(percentage));
}

void MyM5::TableCreate(int tab_idx_, lv_obj_t *table_)
{
  table = table_;
  tab_idx = tab_idx_;
  lv_table_set_cell_value(table, tab_idx, 0, (const char *)txt.c_str());
  lv_table_set_cell_value(table, tab_idx, 1, " -- ");
}

void MyM5::UpdateTable()
{
  switch (dev_idx)
  {
  case MYM5_IDX_BAT:
  { /* Battery Status Indicater */
    lv_table_set_cell_value_fmt(table, tab_idx, 1, (const char *)fmt.c_str(), symbol, voltage, percentage);
  }
  break;
  case MYM5_IDX_WIFI:
  {
    lv_table_set_cell_value(table, tab_idx, 1, symbol.c_str());
  }
  break;
  case MYM5_IDX_DATE:
  {
    lv_table_set_cell_value(table, tab_idx, 1, symbol.c_str());
  }
  break;
  case MYM5_IDX_VBUS:
  {
    lv_table_set_cell_value_fmt(table, tab_idx, 1, (const char *)fmt.c_str(), symbol, voltage, current);
  }
  break;
  case MYM5_IDX_AC:
  {
    lv_table_set_cell_value_fmt(table, tab_idx, 1, (const char *)fmt.c_str(), symbol, voltage, current);
  }
  break;
  case MYM5_IDX_IPS:
  {
    lv_table_set_cell_value_fmt(table, tab_idx, 1, (const char *)fmt.c_str(), voltage);
  }
  break;
  case MYM5_IDX_CHARGE:
  { /* Battery Charging Indicater */
    lv_table_set_cell_value_fmt(table, tab_idx, 1, (const char *)fmt.c_str(), symbol, current, power);
  }
  break;
  case MYM5_IDX_TEMP_APX192:
  {
    lv_table_set_cell_value_fmt(table, tab_idx, 1, (const char *)fmt.c_str(), voltage);
  }
  break;
  case MYM5_IDX_RPM_FAN:
  {
    lv_table_set_cell_value_fmt(table, tab_idx, 1, (const char *)fmt.c_str(), voltage);
  }
  break;
  case MYM5_IDX_RPM_PUMP:
  {
    lv_table_set_cell_value_fmt(table, tab_idx, 1, (const char *)fmt.c_str(), voltage);
  }
  break;
  }
}

/*********************/
/* Feature CLASS DEF */
/*********************/

MyFeature::MyFeature(uint8_t feature_idx_, String txt_, bool state_)
{
  feature_idx = feature_idx_;
  txt = txt_;
  state = state_;
  checkbox = NULL;
  symbol = LV_SYMBOL_MINUS;
}

void MyFeature::DoIt()
{
  switch (feature_idx)
  {
  case FEATURE_IDX_RPM:
  {
  }
  break;
  case FEATURE_IDX_FILT:
  {
  }
  break;
  case FEATURE_IDX_ALIAS:
  {
  }
  break;
  case FEATURE_IDX_ZERO:
  {
  }
  break;
  case FEATURE_IDX_ZOOM:
  {
    if (!state)
      lv_canvas_fill_bg(canvas, lv_palette_lighten(LV_PALETTE_GREY, 3), LV_OPA_COVER);
  }
  break;
  case FEATURE_IDX_HOLD:
  {
  }
  break;
  case FEATURE_IDX_STREAM:
  {
  }
  break;
  case FEATURE_IDX_STATS:
  {
  }
  break;
  case FEATURE_IDX_SERIAL:
  {
  }
  break;
  case FEATURE_IDX_CAL:
  {
  }
  break;
  case FEATURE_IDX_AUTOSCALE:
  {
     if (!state)
    {
      color_scale_max = (uint16_t)lv_slider_get_value(slider_color_scale_max);
      color_scale_min = (uint16_t)lv_slider_get_left_value(slider_color_scale_max);
    }
  }
  break;
  case FEATURE_IDX_PERSISTANT:
  {
    if(SCOPE) memset(SCOPE->persistance, 0, sizeof(SCOPE->persistance)); // ZZZZZ
  }
  break;
  case FEATURE_IDX_DEBUG:
  {
  }
  break;
  case FEATURE_IDX_WIFI:
  {
    if (state)
    {
      if(SetupWifi()){ // if unsuccsessfull
        state = 0;
        Update();
      } 
//      else if(SCOPE) SCOPE->graph_idx = SCOPE_TYPE_VIDEO;
    }
    else
    {
      WiFi.mode(WIFI_OFF);
      ArduinoOTA.end();
      server.end();
//      if(SCOPE) SCOPE->graph_idx = SCOPE_TYPE_VIDEO;
    }
  }
  break;
  case FEATURE_IDX_LDO:
  {
    if (state)
    {
      if (LDO_FAN_INT)
        lv_obj_clear_state(LDO_FAN_INT->slider, LV_STATE_DISABLED);
    }
    else
    {
      if (LDO_FAN_INT)
        lv_obj_add_state(LDO_FAN_INT->slider, LV_STATE_DISABLED);
    }
    if (LDO_FAN_INT)
      M5.Axp.SetLDOVoltage(LDO_FAN_INT->ldo_chan, LDO_FAN_INT->norm * LDO_FAN_INT->cnt * 1000);
    if (LDO_FAN_INT)
      M5.Axp.SetLDOEnable(LDO_FAN_INT->ldo_chan, state);
  }
  break;
  case FEATURE_IDX_PID:
  {
  }
  break;
  case FEATURE_IDX_BIST:
  {
    //  ASIC_SARRAY->EnableBistPad(state);
  }
  break;
  case FEATURE_IDX_ENABLE_SLIDER:
  {
    if (state)
    {
      if (DAC_DET_GATE)
        lv_obj_clear_state(DAC_DET_GATE->slider, LV_STATE_DISABLED);
      if (DAC_VREF)
        lv_obj_clear_state(DAC_VREF->slider, LV_STATE_DISABLED);
      if (DAC_VBB_X2)
        lv_obj_clear_state(DAC_VBB_X2->slider, LV_STATE_DISABLED);
      if (DAC_NC)
        lv_obj_clear_state(DAC_NC->slider, LV_STATE_DISABLED);
      if (POT_REF)
        lv_obj_clear_state(POT_REF->slider, LV_STATE_DISABLED);
      if (POT_DET)
        lv_obj_clear_state(POT_DET->slider, LV_STATE_DISABLED);
      if (POT_VCO)
        lv_obj_clear_state(POT_VCO->slider, LV_STATE_DISABLED);
    }
    else
    {
      if (DAC_DET_GATE)
        lv_obj_add_state(DAC_DET_GATE->slider, LV_STATE_DISABLED);
      if (DAC_VREF)
        lv_obj_add_state(DAC_VREF->slider, LV_STATE_DISABLED);
      if (DAC_VBB_X2)
        lv_obj_add_state(DAC_VBB_X2->slider, LV_STATE_DISABLED);
      if (DAC_NC)
        lv_obj_add_state(DAC_NC->slider, LV_STATE_DISABLED);
      if (POT_REF)
        lv_obj_add_state(POT_REF->slider, LV_STATE_DISABLED);
      if (POT_DET)
        lv_obj_add_state(POT_DET->slider, LV_STATE_DISABLED);
      if (POT_VCO)
        lv_obj_add_state(POT_VCO->slider, LV_STATE_DISABLED);
    }
  }
  break;
  case FEATURE_IDX_CHOPPER:
  {
    if (state)
    {
      /*
                detachInterrupt(GPIO_CLK_CHOP_IN);
                PWM_CHOP->AttachPin();
                if(PWM_CHOP) lv_obj_clear_state(PWM_CHOP->slider, LV_STATE_DISABLED);
                ledcWrite(PWM_CH_CHOP, PWM_RES/2); // 50% duty cycle
      */
    }
    else
    {
      if (PWM_CHOP)
        lv_obj_add_state(PWM_CHOP->slider, LV_STATE_DISABLED);
      /*
                ledcWrite(PWM_CH_CHOP, PWM_RES); // 100% duty cycle
                PWM_CHOP->DetachPin();
                pinMode(GPIO_CLK_CHOP_IN, INPUT_PULLUP);
                attachInterrupt(GPIO_CLK_CHOP_IN, SourceToggle, RISING);
                pinMode(GPIO_SARRAY_CLK_CHOP, OUTPUT);
                en_source = true;
                digitalWrite(GPIO_SARRAY_CLK_CHOP, en_source); // make sure we do not end up with a dark source...
      */
    }
  }
  break;
  case FEATURE_IDX_VPORCH:
  {
    camera->WriteMem(ASIC_CAM_VPORCH, state);
  }
  break;
  case FEATURE_IDX_VC:
  {
    camera->WriteMem(ASIC_CAM_VC_EN, state);
  }
  break;
  case FEATURE_IDX_SUBST:
  {
    camera->WriteMem(ASIC_CAM_SUBSTRACT_EN, state);
  }
  break;
  case FEATURE_IDX_CHOPPER_SYNC:
  {
    // the chopper_out_pad to source = (chopper_select[1] == 1) ? chopper_in_pad : chopper_int;
    // the used chopper of camera array = (chopper_select[0] == 1) ?  chopper_in_pad : chopper_int;
    if (state)
      camera->WriteMem(ASIC_CAM_CHOPPER_SELECT, 0x00);
    else
      camera->WriteMem(ASIC_CAM_CHOPPER_SELECT, 0x02);
  }
  break;
 case FEATURE_IDX_WRITE_INT16:
  {
  }
  break;
  }
  my_debug("DoIt FEATURE#: " + String(feature_idx) + " state: " + String(state));
}

void MyFeature::cb(lv_event_t *event)
{
  lv_event_code_t code = lv_event_get_code(event);
  lv_obj_t *obj = lv_event_get_target(event);
  MyFeature *myClass = (MyFeature *)lv_event_get_user_data(event);
  if (code == LV_EVENT_VALUE_CHANGED)
  {
    myClass->state = lv_obj_get_state(obj) & LV_STATE_CHECKED ? true : false;
    myClass->DoIt();
  }
}

void MyFeature::CheckboxCreate(int posy, lv_obj_t *obj)
{
  checkbox = lv_checkbox_create(obj);
  lv_checkbox_set_text(checkbox, (const char *)txt.c_str());
  if (state) lv_obj_add_state(checkbox, LV_STATE_CHECKED);
  lv_obj_add_event_cb(checkbox, this->cb, LV_EVENT_ALL, this);
  lv_obj_set_pos(checkbox, 20, posy);
}

void MyFeature::Hide()
{
  lv_obj_add_flag(checkbox, LV_OBJ_FLAG_HIDDEN);
}

void MyFeature::UnHide()
{
  lv_obj_clear_flag(checkbox, LV_OBJ_FLAG_HIDDEN);
}

void MyFeature::Update()
{
  if (state)
    lv_obj_add_state(checkbox, LV_STATE_CHECKED);
  else
  {
    lv_obj_clear_state(checkbox, LV_STATE_CHECKED);
  }
}

/*********************/
/* MyGraph CLASS DEF */
/*********************/

MyGraph::MyGraph(uint8_t graph_idx_, String txt_, lv_obj_t *canvas_, MyADC *adc_)
{
  graph_idx = graph_idx_;
  txt = txt_;
  canvas = canvas_;
  adc = adc_;
  cnt = 0;
}

/*********************/
/* MyScope CLASS DEF */
/*********************/

MyScope::MyScope(uint8_t graph_idx_, String txt_, lv_obj_t *canvas_, MyADC *adc_) : MyGraph(graph_idx_, txt_, canvas_, adc_)
{
  lv_draw_line_dsc_init(&line_dsc);
  lv_draw_label_dsc_init(&label_dsc);
  label_dsc.color = lv_palette_main(LV_PALETTE_ORANGE);
  line_dsc.width = 1;
  lv_draw_rect_dsc_init(&rect_dsc);
  rect_dsc.border_width = 5;
  min = 0; // ADC_RANGE_AD7924;
  max = UINT32_MAX; // RANGE_TWID02;
  img_alg_type = IMG_ALG_TYPE_RAW; // just show raw data
  avr_cnt = 1;
  avr_type = false; // true=frame or false=pixel averaging
  shift = (uint8_t)log2(avr_cnt) / 2 + 0.5;
  memset(offset, 0, sizeof(offset));
//  fb_offset = NULL;
//  fb_peak = NULL;
  memset(peak, RANGE_TWID02, sizeof(offset));
  memset(persistance, 0, sizeof(persistance));

//  memset(norm, 1, sizeof(offset));
  max_idx_col = 0;
  max_idx_row = 0;
  mean = 0;
  stdev = 1.0;
}

void MyScope::DoIt()
{
  switch (graph_idx)
  {
  case SCOPE_TYPE_TIME:
  {
    TimeDomain();
  }
  break;
  case SCOPE_TYPE_FREQ:
  {
    Histogram(FFT_TYPE_NORMAL);
  }
  break;
  case SCOPE_TYPE_SPEC:
  {
    Spectrogram();
  }
  break;
  case SCOPE_TYPE_VIDEO:
  {
    Video();
  }
  break;
  case SCOPE_TYPE_STREAM:
  {
//    Video(out);
//    Stream(out);
  }
  break;
  default:
  break;
  }
  my_debug("DoIt MyScope#: " + String(graph_idx) + " state: " + String(graph_idx));
}

void MyScope::TimeDomain()
{ 
  camera_fb_t *fb = SCOPE->FbGet(OR_DEFAULT);
  SCOPE->ImageProcessing(fb, SCOPE->offset, SCOPE->peak, false);
//  uint32_t* fb_uint32Ptr = reinterpret_cast<uint32_t*>(fb->buf);
  int32_t* fb_int32Ptr = reinterpret_cast<int32_t*>(fb->buf);
  uint16_t idx = XRES*cursor_row+cursor_col;
  if (fb_int32Ptr[idx] > chart_max)
  {
    chart_max = fb_int32Ptr[idx];
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, chart_min, chart_max*1.1);
//    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_Y, 10, 5, 6, 5, 1, chart_max/150); //draw_size -- extra size required to draw the tick and labels (start with 20 px and increase if the ticks/labels are clipped) 
  }
  else if (fb_int32Ptr[idx] < chart_min)
  {
    chart_min = fb_int32Ptr[idx];
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, chart_min, chart_max*1.1);
  }
  lv_chart_set_next_value(chart, ser, fb_int32Ptr[idx]);
  //lv_obj_refresh_ext_draw_size(chart);
  lv_chart_refresh(chart);
  esp_camera_fb_return(fb);
}

void MyScope::cb(lv_event_t *event)
{
}

void MyScope::Histogram(uint8_t type)
{ 
  camera_fb_t *fb = SCOPE->FbGet(OR_DEFAULT);
  SCOPE->ImageProcessing(fb, SCOPE->offset, SCOPE->peak, false);
//  SCOPE->ImageProcessing(fb);
  int32_t* fb_int32Ptr = reinterpret_cast<int32_t*>(fb->buf);
  uint16_t my_idx = XRES*cursor_col+cursor_row;

  lv_coord_t * histo_array = lv_chart_get_y_array(histo, histo_ser);
  uint8_t idx = constrain(map(fb_int32Ptr[my_idx], 0.9*min, 1.1*max, 0, HISTOGRAM_RANGE_X-1), 0, HISTOGRAM_RANGE_X-1); // min and max counts are derived from the video shown before histo button is pressed 
  my_debug("MyScope::Histogram: " + String(fb_int32Ptr[my_idx]) + " max: " + String(max) + " min: " + String(min));
  if(histo_array[idx] < HISTOGRAM_RANGE_Y){ // stop if histo gets too high
    histo_array[idx] += 1; // count up
    if(histo_array[idx]>histo_max){ // auto scale yaxis
      histo_max = histo_array[idx];
      lv_chart_set_range(histo, LV_CHART_AXIS_PRIMARY_Y, 0, histo_max);
    } 
    lv_chart_refresh(histo);
  }
  esp_camera_fb_return(fb);
}

void MyScope::Spectrogram()
{
  /*  lv_color_t color;
    adc->BeginTransaction();
    adc->DoItFast(); // do extra read to select channel
    for(uint8_t t=0;t<CANVAS_WIDTH;t++){ // spectrogram lines
      // acqure FFT_SAMPLES
      for(int j=0;j<FFT_SAMPLES;j++){
        adc->DoItFast();
        vReal[j] = adc->cnt;
        vImag[j] = 0;
      }
      // compute FFT
      FFT.Windowing(vReal, FFT_SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);  // Weigh data
      FFT.Compute(vReal, vImag, FFT_SAMPLES, FFT_FORWARD); // Compute FFT
      FFT.ComplexToMagnitude(vReal, vImag, FFT_SAMPLES); // Compute magnitudes
      for(uint8_t j=0;j<CANVAS_WIDTH;j++){// array only contains data up to 1/2 sampling freq. (Nyquist)
        color = ColorMap16bitRGB(color_map, int(vReal[j]), color_scale_min, color_scale_max);
        lv_canvas_set_px_color(canvas, t, CANVAS_HEIGHT -j -1, color); //lv_color_hex(0xFF0000)
      }
      lv_timer_handler(); // make sure lvgl does not hang for too long
      if(graph_idx != SCOPE_TYPE_SPEC) return; // if the user decided to move to another graph type, return
    }
    adc->EndTransaction();
  */
}

esp_err_t MyScope::StatsOnFrame(uint8_t *buf){
  if (!buf)
  {
    log_e("Empty video buffer");
    return(ESP_FAIL);
  }
  int32_t* fb_int32Ptr = reinterpret_cast<int32_t*>(buf);
  float my_sum = 0;
  float my_sum2 = 0;
  size_t idx = 0;
  max = 0;
  min = INT16_MAX;
  for (uint8_t row = 0; row < YRES; row++)
  {
    for (uint8_t col = 0; col < XRES; col++)
    {
      idx = row * YRES + col;
      my_sum += fb_int32Ptr[idx];
      my_sum2 += fb_int32Ptr[idx]*fb_int32Ptr[idx];
      if (fb_int32Ptr[idx] > max)
      {
        max = fb_int32Ptr[idx];
        max_idx_col = col;
        max_idx_row = row;
      }
      if (fb_int32Ptr[idx] < min)
      {
        min = fb_int32Ptr[idx];
      }
    }
  }
  mean = my_sum/(XRES*YRES);
  stdev = sqrt((my_sum2/(XRES*YRES)) - mean * mean);
  return(ESP_OK); 
}

camera_fb_t *MyScope::FbGet(uint8_t orientation)
{
  //ESP_LOGI(TAG, "Switch Orientation %d", orientation);
  camera_fb_t * fb = esp_camera_fb_get(orientation);
  
  if(avr_cnt>1){
    ESP_LOGD(TAG, "average %d frames", avr_cnt);
//    uint8_t *buf = (uint8_t *)ps_malloc(fb->len * sizeof(uint8_t));  // Create the buffer in PSRAM
    uint8_t *buf = (uint8_t *)malloc(fb->len * sizeof(uint8_t)); // Create the buffer in internal RAM, faster
    int32_t *buf_int32Ptr = (int32_t*) buf;
    int32_t *fb_int32Ptr;
    memcpy(buf, fb->buf, fb->len);
    esp_camera_fb_return(fb);
    ESP_LOGD(TAG, "copied 1st frame");
    for(uint8_t i=1; i<avr_cnt; i++){
      fb = esp_camera_fb_get(orientation);
      fb_int32Ptr =  reinterpret_cast<int32_t*>(fb->buf);
      ESP_LOGD(TAG, "add frame %d", i);
      for(size_t j=0;j<fb->len/4;j++){
        buf_int32Ptr[j]+=fb_int32Ptr[j];
      }
      if(i!= (avr_cnt-1)) esp_camera_fb_return(fb);
    }
//    fb_int32Ptr = reinterpret_cast<int32_t*>(fb->buf);
    ESP_LOGD(TAG, "shift and copy back");
    for(size_t j=0;j<fb->len/4;j++){
      fb_int32Ptr[j] = buf_int32Ptr[j]; // without shift
//      fb_int32Ptr[j] = buf_int32Ptr[j] >> shift; // with shift
    }
    free(buf);
  }

//  if(FEATURE_PERSISTANT->state){
//    SCOPE->AddToPersistance(fb); 
//  }

  return(fb);
}

// This function eats up all the time!!! 544fps->10fps
// min max is recalculated every frame and applys to the next frame for speed considerations
esp_err_t MyScope::Video()
{
  if(0){ //check fps limits
  pinMode(3, OUTPUT);
  int num_frames = 100;
  int64_t fr_start = esp_timer_get_time(); // in microseconds
  for(int i = 0; i<num_frames; i++){
    digitalWrite(3, HIGH);
    camera_fb_t * fb = esp_camera_fb_get(OR_ROTATE90_LEFT);
    esp_camera_fb_return(fb);
    digitalWrite(3, LOW);
//    ets_delay_us(100);
  }
  int64_t fr_end = esp_timer_get_time();
  // Calculate the CPU time elapsed in seconds
  double frame_time = (fr_end - fr_start)/1e6;
//  frame_time /= 1000000;
//  printf("Elapsed time: %f seconds\n", elapsed_time);
  Serial.printf("fps: %f time: %f\n", (double) num_frames/frame_time, frame_time);

  return(ESP_OK);
  }

  uint8_t orientation;
  if(FEATURE_HMIRROR->state && FEATURE_VMIRROR->state) orientation = OR_ROTATE90_LEFT;
  else if(FEATURE_VMIRROR->state) orientation = OR_VMIRROR;
  else if(FEATURE_HMIRROR->state) orientation = OR_HMIRROR;
  else if(FEATURE_ROTATE90->state) orientation = OR_ROTATE90_RIGHT;
  else orientation = OR_DEFAULT;

  camera_fb_t *fb = FbGet(orientation);
  if (!fb)
  {
    log_e("Camera image capture failed");
    return(ESP_FAIL);
  }

#if CONFIG_BILINEAR_INTERPOLATION
  if (FEATURE_INTERPOL->state) SCOPE->ImageProcessing(fb, SCOPE->offset, SCOPE->peak, true);
  else SCOPE->ImageProcessing(fb, SCOPE->offset, SCOPE->peak, true);
#else
  SCOPE->ImageProcessing(fb, SCOPE->offset, SCOPE->peak, true);
#endif

  int32_t* fb_int32Ptr = reinterpret_cast<int32_t*>(fb->buf);
  uint32_t* fb_uint32Ptr = reinterpret_cast<uint32_t*>(fb->buf);
  size_t idx = 0;
  
  for (byte row = 0; row < YRES; row++)
  { 
    //alv_timer_handler(); // response too slow otherwise
    for (byte col = 0; col < XRES; col++)
    { 
      idx = XRES*row+col;

#if CONFIG_BILINEAR_INTERPOLATION
      if (FEATURE_INTERPOL->state)
      {
        byte lastrowfix;
        byte lastcolfix;
        if(row==XRESY-1) lastrowfix = row-1;
        else lastrowfix = row;
        if(col==XRESY-1) lastcolfix = col-1;
        else lastcolfix = col;
        size_t idx_tr = XRESY*lastrowfix+(lastcolfix+1);
        size_t idx_bl = XRESY*(lastrowfix+1)+lastcolfix;
        size_t idx_br = XRESY*(lastrowfix+1)+(lastcolfix+1);

        for (byte irow = 0; irow < PIXEL_HEIGHT; irow++)
        {
          for (byte icol = 0; icol < PIXEL_WIDTH; icol++)
          {
            uint32_t signal = bilinearInterpolation(abs(fb_int32Ptr[idx]), abs(fb_int32Ptr[idx_tr]), abs(fb_int32Ptr[idx_bl]), abs(fb_int32Ptr[idx_br]), (double)irow / (PIXEL_HEIGHT - 1), (double)icol / (PIXEL_WIDTH - 1));
//              lv_color_t color.full = bilinearInterpolationRGB(lv_color_hex(fb_uint32Ptr[idx]), lv_color_hex(fb_uint32Ptr[idx_tr]), lv_color_hex(fb_uint32Ptr[idx_bl]), lv_color_hex(fb_uint32Ptr[idx_br]), (double)irow / (PIXEL_HEIGHT - 1), (double)icol / (PIXEL_WIDTH - 1));
            lv_color_t color = lv_color_hex(ColorMap32bitRGB(color_map, signal, color_scale_min, color_scale_max));
            lv_canvas_set_px(canvas, col * PIXEL_WIDTH + icol, row * PIXEL_HEIGHT + irow, color);              
          }
        }    
      }else{
#endif
      rect_dsc.bg_color =  lv_color_hex(fb_uint32Ptr[idx]); 
      if ((row == cursor_row) && (col == cursor_col)){
        rect_dsc.border_width = 2;
        rect_dsc.border_color = lv_color_hex(0xff0000);
      }else rect_dsc.border_width = 0;
      if (FEATURE_ZOOM->state){
          lv_canvas_draw_rect(canvas, col * PIXEL_WIDTH, row * PIXEL_HEIGHT, PIXEL_WIDTH, PIXEL_HEIGHT, &rect_dsc);
      }else lv_canvas_set_px(canvas, YRES * (PIXEL_HEIGHT-1) / 2 + col, XRES * (PIXEL_WIDTH -1) / 2  + row, rect_dsc.bg_color);
#if CONFIG_BILINEAR_INTERPOLATION      
    }
#endif
    }
  }
  esp_camera_fb_return(fb);
  return(ESP_OK);
  //if (FEATURE_INTERPOL->state) free(buf);
}

void MyScope::AddToPersistance(camera_fb_t *fb){ // YYYYY
  int32_t* persistance_int32Ptr = reinterpret_cast<int32_t*>(SCOPE->persistance);
  int32_t* fb_int32Ptr = reinterpret_cast<int32_t*>(fb->buf);
  for (size_t i = 0; i < fb->len/4; ++i) {
    if (fb_int32Ptr[i] > persistance_int32Ptr[i]) {
      persistance_int32Ptr[i] = fb_int32Ptr[i]; 
    } else{
      fb_int32Ptr[i] = persistance_int32Ptr[i];
    }
  }
}
 
lv_color_t MyScope::bilinearInterpolationRGB(lv_color_t tl, lv_color_t tr, lv_color_t bl, lv_color_t br, double x_ratio, double y_ratio) {
    lv_color_t result;
    
    // Calculate weighted averages for each channel
    result.ch.red = (unsigned char)((1 - x_ratio) * (1 - y_ratio) * tl.ch.red + x_ratio * (1 - y_ratio) * tr.ch.red +
                               (1 - x_ratio) * y_ratio * bl.ch.red + x_ratio * y_ratio * br.ch.red);
    
    result.ch.green = (unsigned char)((1 - x_ratio) * (1 - y_ratio) * tl.ch.green + x_ratio * (1 - y_ratio) * tr.ch.green +
                               (1 - x_ratio) * y_ratio * bl.ch.green + x_ratio * y_ratio * br.ch.green);
    
    result.ch.blue = (unsigned char)((1 - x_ratio) * (1 - y_ratio) * tl.ch.blue + x_ratio * (1 - y_ratio) * tr.ch.blue +
                               (1 - x_ratio) * y_ratio * bl.ch.blue + x_ratio * y_ratio * br.ch.blue);
    
    return result;
}


// Bilinear Interpolation Function
uint32_t MyScope::bilinearInterpolation(uint32_t topLeft, uint32_t topRight, uint32_t bottomLeft, uint32_t bottomRight, double x, double y)
{
  double value = (1.0 - x) * (1.0 - y) * topLeft +
                 x * (1.0 - y) * topRight +
                 (1.0 - x) * y * bottomLeft +
                 x * y * bottomRight;
  return (uint32_t)value;
}

esp_err_t IRAM_ATTR MyScope::BackupFrame(uint8_t *buf){
  esp_err_t res = ESP_OK;
    //esp_camera_fb_return(fb_offset); // free old zero
//    uint16_t* offset_int16Ptr = reinterpret_cast<uint16_t*>(fb_offset->buf);
//    uint16_t* peak_uint16Ptr = reinterpret_cast<uint16_t*>(fb_peak->buf);
    camera_fb_t *fb = FbGet(OR_DEFAULT); // esp_camera_fb_get();
    if (!fb)
    {
      log_e("Camera capture failed");
      res = ESP_FAIL;
    }
//    uint16_t* fb_uint16Ptr = reinterpret_cast<uint16_t*>(fb->buf);
    memcpy(buf, fb->buf, fb->len);
    esp_camera_fb_return(fb);
//    for (size_t i = 0; i < fb->len/2; ++i) {
//      fb_uint16Ptr[i] -= offset_int16Ptr[i];
//      fb_uint16Ptr[i] = (fb_uint16Ptr[i] > offset[i]) ? (fb_uint16Ptr[i] - offset[i]) : (offset[i] - fb_uint16Ptr[i]);
//    }
  return(res);
}

esp_err_t IRAM_ATTR MyScope::LoadFrame(uint8_t *buf) {
    esp_err_t res = ESP_OK;
    preferences.begin("storage", true); // Open NVS in read-only mode

    // Read frame data
    size_t len = preferences.getBytes("frame", buf, NUMX * NUMY * 4);
    if (len != NUMX * NUMY * 4) { // Ensure we got the full frame
        res = ESP_FAIL;
        my_debug("Failed to load complete frame from NVS!");
    } else {
        my_debug("Successfully loaded frame from NVS!");
    }

    // Read additional configuration data
    CAMERA_EXPOSER->cnt = preferences.getUInt("exposer", CAMERA_EXPOSER->cnt);
    CAMERA_EXPOSER->DoIt();
    CAMERA_EXPOSER->UpdateSlider();

    #if CONFIG_TWID02_SUPPORT
    #elif CONFIG_TWID03_SUPPORT
    MEM_ROW_MIN_MAX->cnt = preferences.getUInt("MEM_ROW_MIN_MAX", MEM_ROW_MIN_MAX->cnt);
    MEM_ROW_MIN_MAX->DoIt();
    MEM_ROW_MIN_MAX->UpdateSlider();

    MEM_COL_MIN_MAX->cnt = preferences.getUInt("MEM_COL_MIN_MAX", MEM_COL_MIN_MAX->cnt);
    MEM_COL_MIN_MAX->DoIt();
    MEM_COL_MIN_MAX->UpdateSlider();
    #elif CONFIG_TWID04_SUPPORT
    #else
    #endif

    uint16_t val = preferences.getUInt("chopper_div", 0); // Ensure default value
    lv_roller_set_selected(roller_chopper_cyc, val, LV_ANIM_OFF); 

    preferences.end(); // Close NVS
    return res;
}

esp_err_t IRAM_ATTR MyScope::SaveFrame(uint8_t *buf) {
//    nvs_flash_erase();
//    nvs_flash_init();
    preferences.begin("storage", false); // Open NVS (writable)

    // Store frame data
    size_t written = preferences.putBytes("frame", buf, NUMX * NUMY * 4);
    if (written != NUMX * NUMY * 4) {
        my_debug("Failed to store complete frame in NVS!");
        preferences.end();
        return ESP_FAIL;
    }
    my_debug("Successfully stored frame in NVS!");

    // Store additional configuration data
    preferences.putUInt("exposer", CAMERA_EXPOSER->cnt);


    #if CONFIG_TWID02_SUPPORT
    #elif CONFIG_TWID03_SUPPORT
    preferences.putUInt("MEM_ROW_MIN_MAX", MEM_ROW_MIN_MAX->cnt);
    preferences.putUInt("MEM_COL_MIN_MAX", MEM_COL_MIN_MAX->cnt);
    #elif CONFIG_TWID04_SUPPORT
    #else
    #endif

    preferences.putUInt("chopper_div", lv_roller_get_selected(roller_chopper_cyc));

    preferences.end(); // Close NVS
    return ESP_OK;
}

esp_err_t IRAM_ATTR MyScope::ImageProcessing(camera_fb_t *fb, uint8_t *buf_offset, uint8_t *buf_peak, bool convert2rgb)
{ 
  esp_err_t res = ESP_OK;
  if (!fb)
  {
    log_e("Camera image processing failed");
    return(ESP_FAIL);
  }
  uint32_t my_min = 0xFFFFFFF; 
  uint32_t my_max = 0;
  uint32_t my_abs = 0;
  float my_peak = 1;

  size_t idx_cursor = XRES*cursor_row+cursor_col;
//  ESP_LOGD(TAG,"raw: LB %d HB %d", fb->buf[idx_cursor*2], fb->buf[idx_cursor*2+1]);
  int32_t* fb_int32Ptr = reinterpret_cast<int32_t*>(fb->buf);
  uint32_t* fb_uint32Ptr = reinterpret_cast<uint32_t*>(fb->buf);
  int32_t* offset_int32Ptr = reinterpret_cast<int32_t*>(buf_offset);
  int32_t* peak_int32Ptr = reinterpret_cast<int32_t*>(buf_peak);
  int32_t* persistance_int32Ptr = reinterpret_cast<int32_t*>(SCOPE->persistance);

//  ESP_LOGD(TAG,"raw: %d", fb_int32Ptr[idx_cursor]);
//  ESP_LOGD(TAG,"raw int: %d", (int32_t) fb_int32Ptr[idx_cursor]);

    for (size_t i = 0; i < fb->len/4; ++i) {
      if(en_zero){
//        if(i==idx_cursor) ESP_LOGD(TAG,"offset: %d", offset_int32Ptr[i]);
        fb_int32Ptr[i] -= offset_int32Ptr[i];
//        if(i==idx_cursor) ESP_LOGD(TAG,"zeroed: %d", fb_int32Ptr[i]);
        if(en_scale){ // peak scaling requires an offset
          my_peak = (float) abs(peak_int32Ptr[i] - offset_int32Ptr[i]);
          my_abs = (uint32_t) UINT16_MAX * abs(fb_int32Ptr[i])/my_peak;
        } else my_abs = abs(fb_int32Ptr[i]); 
      } else my_abs = abs(fb_int32Ptr[i]); 
      
//      ESP_LOGD(TAG, "my_abs %d fb_int32Ptr %d", my_abs, (int32_t) fb_int32Ptr[i]);

  if(FEATURE_PERSISTANT->state){
    if (fb_int32Ptr[i] > persistance_int32Ptr[i]) {
      persistance_int32Ptr[i] = fb_int32Ptr[i]; 
    } else{
      fb_int32Ptr[i] = persistance_int32Ptr[i];
    }
    my_abs = abs(fb_int32Ptr[i]);
  }

      if (FEATURE_AUTOSCALE->state) // search for new max used in next frame
      {
        if (my_abs > my_max) my_max = my_abs;
        if (my_abs < my_min) my_min = my_abs;
//        fb_uint32Ptr[i] = ColorMap32bitRGB(color_map, my_abs, min, max);
//        if(i==idx_cursor) ESP_LOGD(TAG,"RGB auto scaled: 0x%X", fb_uint32Ptr[i]);
      }
      if(i==idx_cursor) cnt = (uint16_t) my_abs; // store the cursor monitor data
      if(convert2rgb) fb_uint32Ptr[i] = ColorMap32bitRGB(color_map, my_abs, color_scale_min, color_scale_max);
          if(i==idx_cursor) ESP_LOGD(TAG,"RGB regular scale: 0x%X", fb_uint32Ptr[i]);      
    }
//  ESP_LOGD(TAG,"RGB565: LB 0x%X HB 0x%X", fb->buf[idx_cursor*2], fb->buf[<idx_cursor>*2+1]);

  if (FEATURE_AUTOSCALE->state){ // fill in the new min/max
    color_scale_min = my_min;
    if((my_max - my_min) < AUTOSCALE_NOISE_THRESHOLD) color_scale_max = my_min + AUTOSCALE_NOISE_THRESHOLD;
    else color_scale_max = my_max;
//    ESP_LOGD(TAG, "max %d min %d", color_scale_max, color_scale_min);
    lv_slider_set_value(slider_color_scale_max, color_scale_max, LV_ANIM_ON);
    lv_slider_set_left_value(slider_color_scale_max, color_scale_min, LV_ANIM_ON);
  }
  min = my_min; // used later in histo, consider using min instead of local var my_min and my_max  
  max = my_max;
  return(res);
}

void MyScope::Save(camera_fb_t *fb, FsFile &csv)
{
  int32_t* fb_int32Ptr = reinterpret_cast<int32_t*>(fb->buf);
  int32_t* offset_int32Ptr = reinterpret_cast<int32_t*>(offset);
  if(csv && fb){
    if(en_zero){
      for (size_t i = 0; i < fb->len/4; ++i) {
        fb_int32Ptr[i] -= offset_int32Ptr[i];
      }
    }
    // Write the frame data to the CSV file
    for (size_t i = 0; i < fb->len/4; i++) {
      csv.print(fb_int32Ptr[i]);      
      if (i % fb->width == (fb->width-1)) {
        csv.println(); // Move to the next line for the next frame
      }else csv.print(',');
    }
  }
}

void MyScope::Send(camera_fb_t *fb)
{
  int32_t* fb_int32Ptr = reinterpret_cast<int32_t*>(fb->buf);
  int16_t* fb_int16Ptr = reinterpret_cast<int16_t*>(fb->buf);
  int32_t* offset_int32Ptr = reinterpret_cast<int32_t*>(offset);
  for (size_t i = 0; i < fb->len/4; ++i) {
    if(en_zero){fb_int32Ptr[i] -= offset_int32Ptr[i];}
    if(FEATURE_WRITE_INT16->state){fb_int16Ptr[i] = static_cast<int16_t>(fb_int32Ptr[i]);}
  }
  
  if(FEATURE_WRITE_INT16->state){ // send int16 (low byte only) 
    MuxSerialToWifi ? client.write(fb->buf, fb->len) : Serial.write(fb->buf, fb->len/2);  
  }else{   // send full int32 data istead
    MuxSerialToWifi ? client.write(fb->buf, fb->len) : Serial.write(fb->buf, fb->len);  
  }
}

void MyScope::Stream(camera_fb_t *fb)
{ 
}

MySPI::MySPI(uint8_t GPIO_RST_, uint8_t GPIO_SS_)
{
  GPIO_RST = GPIO_RST_;
  GPIO_SS = GPIO_SS_;
  spi_clock = ASIC_SPI_Clock;
  bit_order = SPI_MSBFIRST;
  //  data_mode=SPI_MODE0;
  data_mode = SPI_MODE0;
  tx_data = 0;
  rx_data = 0;
  pinMode(GPIO_SS, OUTPUT);
  pinMode(GPIO_RST, OUTPUT); // set reset pin to output
  Reset();
}

void MySPI::Reset()
{
  digitalWrite(GPIO_RST, LOW); // reset is low active
  delay(1);
  digitalWrite(GPIO_RST, HIGH); // apply reset (async reset part)
}

void MySPI::BeginTransaction()
{
  SPI.beginTransaction(SPISettings(spi_clock, bit_order, data_mode)); // MSBFIRST not working
}

void MySPI::EndTransaction()
{
  SPI.endTransaction();
}

void MySPI::DoItFast()
{
  digitalWrite(GPIO_SS, LOW); // pull SS low to prep other end for transfer
  rx_data = SPI.transfer(tx_data);
  digitalWrite(GPIO_SS, HIGH); // pull ss high to signify end of data transfer
  // cnt = (uint8_t) rx_data & 0xFF;
}

void MySPI::DoIt()
{
  my_debug("ASIC DoIt: send " + String(tx_data));
  SPI.beginTransaction(SPISettings(spi_clock, bit_order, data_mode)); // MSBFIRST not working
  digitalWrite(GPIO_SS, LOW);                                         // pull SS low to prep other end for transfer
  rx_data = SPI.transfer(tx_data);
  digitalWrite(GPIO_SS, HIGH); // pull ss high to signify end of data transfer
  SPI.endTransaction();
  // cnt = (uint16_t) rx_data; //(uint8_t) (rx_data) & 0xFF;
  my_debug("ASIC DoIt: received " + String(rx_data));
}

void MySPI::WriteMem(uint8_t addr, uint8_t data)
{
  tx_data = ASIC_CAM_W | addr;
  DoIt();
  tx_data = data;
  DoIt();
}

uint8_t MySPI::ReadMem(uint8_t addr)
{
  tx_data = ASIC_CAM_R | addr;
  DoIt();
  tx_data = 0;
  DoIt();
  return (rx_data);
}

/*********************/
/* tw_id02 CLASS DEF */
/*********************/

tw_id02::tw_id02(const int xres_, const int yres_, const int CAM_RESET, const int VSYNC, const int HREF, uint8_t XCLK, const int PCLK, const int D0, const int D1, const int D2, const int D3, const int D4, const int D5, const int D6, const int D7) : MySPI(GPIO_CAMERA_RST, GPIO_CAMERA_SS)
{
  Reset();
  xres=xres_;
  yres=yres_;
}

int tw_id02::init()
{
  frameSize = XRES * YRES * BYTES_PER_PIXEL;
  CANVAS_WIDTH = PIXEL_WIDTH * XRES; //should not exeed 240
  CANVAS_HEIGHT = PIXEL_HEIGHT * YRES; //should not exeed 240
  xres = XRES;
  yres = YRES;
//  camera_config.row_min = 0;
//  camera_config.row_max = YRES;
//  camera_config.col_min = 0;
//  camera_config.col_max = XRES;
  esp_err_t err = esp_camera_deinit();
  //ESP_LOGE("esp_camera_deinit:", "%d", err);
  err = esp_camera_init(&camera_config);
  //ESP_LOGE("esp_camera_init:", "%d", err);

  BMP::construct16BitHeader(bmpHeader, xres, yres);
  if (cbuf != NULL) {
    free(cbuf);
    cbuf = NULL;
    my_debug("Canvas buffer freed\n");
  }
//  cbuf = (lv_color_t *)ps_malloc(CANVAS_WIDTH * CANVAS_HEIGHT * sizeof(lv_color_t)); // Create the canvas buffer in PSRAM
  cbuf = (lv_color_t *)malloc(CANVAS_WIDTH * CANVAS_HEIGHT * sizeof(lv_color_t)); // Create the canvas buffer in internal RAM, faster
  if (cbuf == NULL) {
        my_debug("Reallocation of buffer failed\n");
        return -1;
  }
  my_debug((String) "init_canvas: CANVAS_WIDTH: " + String(CANVAS_WIDTH) + " CANVAS_HEIGHT: " + String(CANVAS_HEIGHT));
  lv_canvas_set_buffer(canvas, cbuf, CANVAS_WIDTH, CANVAS_HEIGHT, LV_IMG_CF_TRUE_COLOR);
  lv_obj_set_pos(canvas, int(240 - CANVAS_WIDTH) / 2, int(240 - CANVAS_HEIGHT) / 2); // try to center
  lv_canvas_fill_bg(canvas, lv_palette_lighten(LV_PALETTE_GREY, 3), LV_OPA_COVER); // YYYY
#if CONFIG_TWID03_SUPPORT
  MEM_PCLK_LB->DoIt();
  MEM_PCLK_HB->DoIt();
  CAMERA_XCLK->DoIt();
  MEM_HSYNC->DoIt();
  if(FEATURE_VPORCH) FEATURE_VPORCH->DoIt();
  if(FEATURE_SUBST) FEATURE_SUBST->DoIt();
  if(FEATURE_CHOPPER_SYNC) FEATURE_CHOPPER_SYNC->DoIt();
  uint8_t ChopperDiv = camera->Col2ChopperDiv(xres);
  uint8_t porch = camera->Col2Porch(xres);
  camera->WriteMem(ASIC_CAM_CHOP_DIV, ChopperDiv);
  camera->WriteMem(ASIC_CAM_HPORCH, (porch << 4) | porch);
  if(FEATURE_VC) FEATURE_VC->DoIt();
  my_debug("xres=" + String(xres) + " ChopperDiv=" + String(ChopperDiv) + " FPORCH=BPORCH=" + String(porch));
#endif 
  return(0);
}

  uint8_t tw_id02::Col2ChopperDiv(uint8_t cols) // for only 1 chopper cycle
  {
    return((uint8_t)2*cols+6); 
  }

  uint8_t tw_id02::Col2Porch(uint8_t cols) // for only 1 chopper cycle
  {
    return((cols % 3 == 0) ? 0 : 1);
  }
/*
tw_id02::FindPorch(uint cyc_chopper)
{
uint ROWS = 33;
uint COLS = ROWS;
uint LOWHIGH = 2; // The video controller has to send HIGH and LOW BYTE, so it needs 2*$ROWS pclock cycles in one HREF
uint FPORCH = 1; // front porch = pclock cycles before HSYNC
uint BPORCH = 1; // back porch = pclock cycles after HSYNC, we require BPORCH = FPORCH (require symmetric settings around HREF)
uint CHOPPER_DIV = 1; // chopper divider ratio derived from pclock
uint EXTRA_HSYNC_CYC = 4;// (fixed in this calculation) extra two pclock cycles before and after HREF, e.g. HSYNC has 4 more pclock cycles than HREF

uint CYC_HREF_ONLY = LOWHIGH*ROWS; // pclk cycles for HREF
uint CYC_HSYNC_ONLY = FPORCH+CYC_HREF_ONLY+BPORCH; // pclk cycles for HSYNC
uint CYC_FIXED = CYC_HREF_ONLY + EXTRA_HSYNC_CYC; // pclock cycles which can not be changed
uint CYC_TOTAL = CYC_HSYNC_ONLY + EXTRA_HSYNC_CYC; // pclk cycles for HSYNC and extra HSYNC porches

int remainder;
int CYC_CHOPPER;
for(CHOPPER_DIV = 2; CHOPPER_DIV <= CYC_TOTAL; CHOPPER_DIV=CHOPPER_DIV+2){ // count CHOPPER_DIV in even numbers up

  remainder = CYC_FIXED/CHOPPER_DIV - int(CYC_FIXED/CHOPPER_DIV); // calculate Nachkommastelle, falls remainder=0 => chopper fits into $CYC_HSYNC_ONLY symetrically
  // print "remainder: $remainder\n";

  FPORCH = (1-remainder)*CHOPPER_DIV/2; // calculate how much FPORCH would have to grow to compensate for the remainder
  BPORCH = FPORCH;
  // print "FPORCH: $FPORCH\n";

// Recalculate
  CYC_HSYNC_ONLY = FPORCH+CYC_HREF_ONLY+BPORCH; // pclk cycles for HSYNC
  CYC_TOTAL = CYC_HSYNC_ONLY + EXTRA_HSYNC_CYC; // pclk cycles for HSYNC and extra HSYNC porches
  CYC_CHOPPER = CYC_TOTAL/CHOPPER_DIV;
  // printf "CHOPPER_DIV: %0d (x%0X) CYC_CHOPPER: %0d FPORCH=BPORCH: %0d (x%0X)\n", $CHOPPER_DIV, $CHOPPER_DIV, $CYC_CHOPPER, $FPORCH, $FPORCH;
}
}
*/

/*****************/
/* MEM CLASS DEF */
/*****************/

MyMEM::MyMEM(uint8_t address_, String txt_, String fmt_, uint16_t cnt_, uint16_t cnt_min_, uint16_t cnt_max_, float norm_) : MyDevice(txt_, fmt_, cnt_, cnt_min_, cnt_max_, norm_)
{
  address = address_;
  DoIt(); // write the default
}

MyMEM::~MyMEM()
{
}

void MyMEM::DoIt()
{
 
  switch (address)
  {
    #if CONFIG_TWID02_SUPPORT
    #elif CONFIG_TWID03_SUPPORT
  case ASIC_CAM_ROW_MAX:{
//    camera->WriteMem(ASIC_CAM_VC_EN, 0);
    YRES = cnt-cnt2+1;
    camera_config.row_min = cnt2;
    camera_config.row_max = cnt;
    camera->init();
    camera->WriteMem(address, (uint8_t)cnt); 
    my_debug("MEM DoIt address: " + String(address) + " " + String(cnt) + " " + String(Cnt2Volt(cnt)));
    camera->WriteMem(ASIC_CAM_ROW_MIN, (uint8_t)cnt2); 
    my_debug("MEM DoIt address: " + String(ASIC_CAM_ROW_MIN) + " " + String(cnt2) + " " + String(Cnt2Volt(cnt2)));
//    init_canvas();
//    camera->WriteMem(ASIC_CAM_VC_EN, 1);
  break;
  } 
  case ASIC_CAM_COL_MAX:
//    camera->WriteMem(ASIC_CAM_VC_EN, 0);
    XRES = cnt-cnt2+1;
    camera_config.col_min = cnt2;
    camera_config.col_max = cnt;
    camera->init();
    camera->WriteMem(address, (uint8_t)cnt); 
    my_debug("MEM DoIt address: " + String(address) + " " + String(cnt) + " " + String(Cnt2Volt(cnt)));   
    camera->WriteMem(ASIC_CAM_COL_MIN, (uint8_t)cnt2); 
    my_debug("MEM DoIt address: " + String(ASIC_CAM_COL_MIN) + " " + String(cnt2) + " " + String(Cnt2Volt(cnt2)));    
//    init_canvas();
//    camera->WriteMem(ASIC_CAM_VC_EN, 1);
    break;
    #elif CONFIG_TWID04_SUPPORT
    #else
    #endif

  default:
    camera->WriteMem(address, (uint8_t)cnt);  
    my_debug("MEM DoIt address: " + String(address) + " " + String(cnt) + " " + String(Cnt2Volt(cnt)));
    break;
  }
}

/*********************/
/* EXPOSER CLASS DEF */
/*********************/
// Q: What is the exposer time in ms for CAMERA_XCLK->cnt (in MHz), PCLK divid CAMERA_EXXPOSER->cnt (in counts), and the roller_chopper_cyc setting? 
    // lookupValidChopper provides golden ratios for which the chopper is synced AND phase aligned (equal on/off phases with respect to HSYNC, e.g. offsets after subtract = ~0)
    // => periods are all the same: T_hsync = T_href = T_chopper*chopper_cyc, just with different duty cycles
//    uint8_t chopper_cyc = camera->lookupValidChopper[lv_roller_get_selected(roller_chopper_cyc)][0]; // number of chopper cyc per HSYNC
//    uint8_t chopper_div = camera->lookupValidChopper[lv_roller_get_selected(roller_chopper_cyc)][1]; // rato for which the chopper runs slower than PCLK
//    uint8_t porch = camera->lookupValidChopper[lv_roller_get_selected(roller_chopper_cyc)][2]; // HSYNC is 2*porch PCLK cyc wider than HREF 
//    uint8_t hsync = camera->lookupValidChopper[lv_roller_get_selected(roller_chopper_cyc)][3]; // PCLK cyc for which HSYNC is LOW
// the following should be true:
//    uint16_t HREF_HIGH_CYC = 2*NUMX;
//    uint16_t HSYNC_HIGH_CYC = HREF_HIGH_CYC +2*porch;  
//    uint16_t TOTAL_CYC = HSYNC_HIGH_CYC +hsync; 
//    uint16_t T_PCLK_us = CAMERA_EXPOSER->cnt/CAMERA_XCLK->cnt; // XCLK is in MHz
//    uint16_t T_INTEGRATION_us = (T_PCLK_us * HSYNC_HIGH_CYC)/2; // half for ON, half for off
//    uint16_t T_CHOPPER_us = T_PCLK_us * TOTAL_CYC;
//    uint16_t f_CHOPPER_MHz = 1/T_CHOPPER_us;
//    CAMERA_EXPOSER->cnt = T_INTEGRATION_us *CAMERA_XCLK->cnt *2/HSYNC_HIGH_CYC; // valid for T_INTEGRATION_us = 68-6963.2 µs
//    norm =  CAMERA_XCLK->cnt *2/HSYNC_HIGH_CYC =  CAMERA_XCLK->cnt *2 /(2*NUMX +2*porch) = CAMERA_XCLK->cnt /(NUMX +porch)

MyEXPOSER::MyEXPOSER(String txt_, String fmt_, uint16_t cnt_, uint16_t cnt_min_, uint16_t cnt_max_, float norm_) : MyDevice(txt_, fmt_, cnt_, cnt_min_, cnt_max_, norm_)
{
  DoIt(); // write the default
}

MyEXPOSER::~MyEXPOSER()
{
}

void MyEXPOSER::DoIt()
{
  my_debug("EXPOSER DoIt: " + String(cnt) + " " + String(Cnt2Volt(cnt)));
  uint8_t porch = camera->lookupValidChopper[lv_roller_get_selected(roller_chopper_cyc)][2]; // HSYNC is 2*porch PCLK cyc wider than HREF 
  uint16_t div = (uint16_t) cnt *CAMERA_XCLK->cnt /(NUMX+porch);
  
  MEM_PCLK_LB->cnt = div & 0xFF; // low byte
  MEM_PCLK_LB->DoIt();
  MEM_PCLK_LB->UpdateTable();
  MEM_PCLK_LB->UpdateSlider();
  
  MEM_PCLK_HB->cnt = (div >> 8) & 0x7; // high byte
  MEM_PCLK_HB->DoIt();
  MEM_PCLK_HB->UpdateTable();
  MEM_PCLK_HB->UpdateSlider();

  en_zero = false; // offsets are gone for other exposer times!
  en_zero_save = en_zero; // make sure frame is not saved on power off.
  //lv_btnmatrix_clear_btn_ctrl(btnm3, 1, LV_BTNMATRIX_CTRL_CHECKED);

}

/*****************/
/* PWM CLASS DEF */
/*****************/

MyXCLK::MyXCLK(uint8_t pwm_pin_, uint8_t pwm_chan_, String txt_, String fmt_, uint16_t cnt_, uint16_t cnt_min_, uint16_t cnt_max_, float norm_) : MyDevice(txt_, fmt_, cnt_, cnt_min_, cnt_max_, norm_)
{
  pwm_pin = pwm_pin_;
  pwm_chan = pwm_chan_;
  pwm_freq = (int)1000000; // in Hz
  // pwm_freq= 1000; // in Hz
  cnt = cnt_;

  if (0)
  {
    periph_module_enable(PERIPH_LEDC_MODULE);

    // ledc_timer_config_t timer_conf;
    timer_conf.bit_num = (ledc_timer_bit_t)1;
    timer_conf.freq_hz = pwm_freq;
    timer_conf.speed_mode = LEDC_HIGH_SPEED_MODE;
    timer_conf.timer_num = LEDC_TIMER_0;
    ledc_timer_config(&timer_conf);

    //    ledc_channel_config_t ch_conf;
    ch_conf.channel = (ledc_channel_t)pwm_chan;
    ch_conf.timer_sel = LEDC_TIMER_0;
    ch_conf.intr_type = LEDC_INTR_DISABLE;
    ch_conf.duty = 1;
    ch_conf.speed_mode = LEDC_HIGH_SPEED_MODE;
    ch_conf.gpio_num = pwm_pin;
    ch_conf.hpoint = 0; // added by me
    ledc_channel_config(&ch_conf);
  }
  else
  {
    int bit_width = 1;    // 1 - 20 bits
    int divider = 0x1024; // 10240 // Q10.8 fixed point number, 0x100 — 0x3FFFF
    int duty_cycle = 1 << (bit_width - 1);

    float freq_hz = ((uint64_t)LEDC_APB_CLK_HZ << 8) / (float)divider / (1 << bit_width);
    my_debug("frequency: " +  String(freq_hz) + " Hz\n");

    ledc_timer_set(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0, divider, bit_width, LEDC_APB_CLK);
    ledc_timer_rst(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0);
    ledc_timer_resume(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0);

    ledc_channel_config_t channel_config = {0};
    channel_config.channel = LEDC_CHANNEL_0;
    channel_config.duty = duty_cycle;
    channel_config.gpio_num = pwm_pin;
    channel_config.speed_mode = LEDC_HIGH_SPEED_MODE;
    channel_config.timer_sel = LEDC_TIMER_0;

    ledc_channel_config(&channel_config);
  }
}

MyXCLK::~MyXCLK()
{
}

void MyXCLK::DoIt()
{
  // my_debug("XCLK DoIt timer_conf.freq_hz: " + String(pwm_chan) + " " + String(cnt) + " " + String(Cnt2Volt(cnt)));
  // ClockDisable();
  int32_t fixedValue = static_cast<int32_t>(256.0 * 40 / Cnt2Volt(cnt));
  my_debug("XCLK DoIt timer_conf.freq_hz: " + String(pwm_chan) + " " + String(cnt) + " " + String(Cnt2Volt(cnt)) + " " + String(fixedValue));
  ledc_timer_set(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0, fixedValue, 1, LEDC_APB_CLK); // bitwidth = 1, LEDC_CLK_DIV = A + B/256 with A=cnt<8 and B=0:
                                                                                   //  ledc_timer_set(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0, (uint32_t) cnt  | 0x200, 1, LEDC_APB_CLK); // bitwidth = 1, LEDC_CLK_DIV = A + B/256 with A=cnt<8 and B=0:
  ledc_timer_rst(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0);
  ledc_timer_resume(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0);

  // ClockEnable();
}

void MyXCLK::ClockEnable()
{
  periph_module_enable(PERIPH_LEDC_MODULE);
}

void MyXCLK::ClockDisable()
{
  periph_module_disable(PERIPH_LEDC_MODULE);
}
