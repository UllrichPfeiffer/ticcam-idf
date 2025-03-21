#include "colormap.h"

float linear(float x, float m, float b){ // y=m*x+b with y=[0,1] and x=[0,1]
  return(m*x+b);
}

uint32_t color_make_RGB888(uint8_t R, uint8_t G,uint8_t B){
  return(R<<16 | G<<8 | B);
}

// Cloormap Winter
uint32_t ColorMapWinter(uint32_t gray, uint32_t min, uint32_t max){
  if(gray>max) gray = max-min;
  else if(gray>min) gray=gray-min;
  else gray=0;
  float scale = (max-min);
  uint8_t R = 0;
  uint8_t G = (uint8_t) 255*linear(gray/scale,1.0,0.0);
  uint8_t B = (uint8_t) 255*linear(gray/scale,-0.5,1.0);
  return(color_make_RGB888(R,G,B)); 
}

uint32_t ColorMapSpring(uint32_t gray, uint32_t min, uint32_t max){
  if(gray>max) gray = max-min;
  else if(gray>min) gray=gray-min;
  else gray=0;
  float scale = (max-min);
  uint8_t R = 255; 
  uint8_t G = (uint8_t) 255*linear(gray/scale,1.0,0.0);
  uint8_t B = (uint8_t) 255*linear(gray/scale,-1.0,1.0);
  return(color_make_RGB888(R,G,B)); 
}

uint32_t ColorMapCool(uint32_t gray, uint32_t min, uint32_t max){
  if(gray>max) gray = max-min;
  else if(gray>min) gray=gray-min;
  else gray=0;
  float scale = (max-min);
  uint8_t R = (uint8_t) 255*linear(gray/scale,1.0,0.0);
  uint8_t G = (uint8_t) 255*linear(gray/scale,-1.0,1.0);
  uint8_t B = 255;
  return(color_make_RGB888(R,G,B)); 
}

uint32_t ColorMapAutumn(uint32_t gray, uint32_t min, uint32_t max){
  if(gray>max) gray = max-min;
  else if(gray>min) gray=gray-min;
  else gray=0;
  float scale = (max-min);
  uint8_t R = 255;
  uint8_t G = (uint8_t) 255*linear(gray/scale,1.0,0.0);
  uint8_t B = 0;
  return(color_make_RGB888(R,G,B)); 
}

uint32_t ColorMapJet(uint32_t gray, uint32_t min, uint32_t max) { 
  if(gray>max) gray = max-min;
  else if(gray>min) gray=gray-min;
  else gray=0;
  float scale = max-min;
  uint8_t R = (uint8_t) 255*trapezoid(gray,scale*0.38,scale*0.62,scale*0.88,scale*1.12);
  uint8_t G = (uint8_t) 255*trapezoid(gray,scale*0.12,scale*0.38,scale*0.62,scale*0.88);
  uint8_t B = (uint8_t) 255*trapezoid(gray,-0.12*scale,scale*0.12,scale*0.38,scale*0.62);
  return(color_make_RGB888(R,G,B)); 
}

uint32_t ColorMapGray(uint32_t gray, uint32_t min, uint32_t max) { 
  if(gray>max) gray = max-min;
  else if(gray>min) gray=gray-min;
  else gray=0;
  uint32_t scale = max-min;
  uint8_t R = (uint8_t) 255*gray/scale; // integer divison is OK, first 255*gray then integ dev with scale 
  uint8_t G = (uint8_t) 255*gray/scale;
  uint8_t B = R;
  return(color_make_RGB888(R,G,B)); 
}

uint32_t JetByteToRGB565(byte gray, byte min, byte max) { 
  if(gray>max) gray = max-min;
  else if(gray>min) gray=gray-min;
  else gray=0;
  float scale = max-min;
  uint16_t R = (uint16_t) 31*trapezoid(gray,scale*0.38,scale*0.62,scale*0.88,scale*1.12) +0.5;
  uint16_t G = (uint16_t) 63*trapezoid(gray,scale*0.12,scale*0.38,scale*0.62,scale*0.88) +0.5;
  uint16_t B = (uint16_t) 31*trapezoid(gray,-0.12*scale,scale*0.12,scale*0.38,scale*0.62) +0.5;
  return(R<<11 | G<<5 | B);
}

uint32_t Jet16bitToRGB565(uint32_t gray, uint32_t min, uint32_t max) { 
  if(gray>max) gray = max-min;
  else if(gray>min) gray=gray-min;
  else gray=0;
  float scale = max-min;
  uint16_t R = (uint16_t) 31*trapezoid(gray,scale*0.38,scale*0.62,scale*0.88,scale*1.12) +0.5;
  uint16_t G = (uint16_t) 63*trapezoid(gray,scale*0.12,scale*0.38,scale*0.62,scale*0.88) +0.5;
  uint16_t B = (uint16_t) 31*trapezoid(gray,-0.12*scale,scale*0.12,scale*0.38,scale*0.62) +0.5;
  return(R<<11 | G<<5 | B);
}

float trapezoid(uint32_t x, float min1, float max1, float max2, float min2){ // Red Example: 0.38,0.62,0.88,1.0
  float m1 = 1/(max1-min1); // Steigung (rising edge)
  float m2 = 1/(min2-max2); // Steigung (falling edge)
  if(x<min1 || x>min2){return(0);} // outside trapezoid
  else if(x>max1 && x<max2){return(1);} // inside trapezoid
  else if(x<=max1){return(m1*(x-min1));} // linear fit (rising edge)
  else if(x>=max2){return(1-m2*(x-max2));} // linear fit (falling edge)
  else{
    //my_debug("Unknown color: " + String(x));
    }
  return(0);
}
