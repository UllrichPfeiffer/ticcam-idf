
#ifndef CAM_FILTER_H
#define CAM_FILTER_H

#include <Arduino.h>
#include "main.h"

void insertionSort(uint16_t arr[], int n);
void filter_Frame(uint16_t (&frame) [NUMX][NUMY], uint16_t (&temp) [NUMX][NUMY]);
void filter_Frame1(uint16_t (&frame)[NUMX][NUMY], uint16_t (&temp)[NUMX][NUMY]);
void filter_Frame_Smoothing (uint16_t (&frame)[NUMX][NUMY], uint16_t (&temp)[NUMX][NUMY]);
void schwell_wert_filter(uint16_t (&frame)[NUMX][NUMY], uint16_t (&temp)[NUMX][NUMY]);
void imag(uint16_t (&frame)[NUMX][NUMY], uint16_t (&temp)[NUMX][NUMY]);
uint16_t sobel_Gradient(uint16_t arr [], int hx[], int hy[], uint8_t n );
void sobel_Operator_Filter(uint16_t (&frame)[NUMX][NUMY], uint16_t (&temp)[NUMX][NUMY]);
uint16_t laplace_Gradient(uint16_t arr[], int mask[], int n);
void laplace_filter(uint16_t(&frame)[NUMX][NUMY], uint16_t (&temp)[NUMX][NUMY]);
void laplace_Smoothing_Filter(uint16_t (&frame)[NUMX][NUMY], uint16_t (&temp)[NUMX][NUMY]);
void laplace_Schwell_Wert_Filter(uint16_t (&frame)[NUMX][NUMY], uint16_t (&temp)[NUMX][NUMY]);
void Sobel_Smoothing (uint16_t (&frame)[NUMX][NUMY], uint16_t (&temp)[NUMX][NUMY]);
void smooting_schwell_Filter(uint16_t (&frame)[NUMX][NUMY], uint16_t (&temp)[NUMX][NUMY]);
void Sobel_Smoothing_Schwellwert_Filter(uint16_t (&frame)[NUMX][NUMY], uint16_t (&temp)[NUMX][NUMY]);

#endif