
#ifndef COLORMAP_H
#define COLORMAP_H

#include <Arduino.h>
#include "lvgl.h"

uint32_t color_make_RGB888(uint8_t R, uint8_t G,uint8_t B);
uint32_t ColorMapWinter (uint32_t gray, uint32_t min, uint32_t max);
uint32_t ColorMapAutumn(uint32_t gray, uint32_t min, uint32_t max);
uint32_t ColorMapCool(uint32_t gray, uint32_t min, uint32_t max);
uint32_t ColorMapSpring(uint32_t gray, uint32_t min, uint32_t max);
uint32_t ColorMapJet(uint32_t gray, uint32_t min, uint32_t max);
uint32_t ColorMapGray(uint32_t gray, uint32_t min, uint32_t max);
float linear (float x, float m, float y);
float trapezoid(uint32_t x, float min1, float max1, float max2, float min2);
uint16_t JetByteTo16bit(byte gray, byte min, byte max);
uint16_t Jet12bitTo16bit(uint16_t gray, uint16_t min, uint16_t max); 

#endif