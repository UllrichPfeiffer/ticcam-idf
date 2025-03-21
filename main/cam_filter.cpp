#include "cam_filter.h"

/* Function to sort an array using insertion sort*/
void insertionSort(uint16_t arr[], int n) {
  int i, key, j;
  for (i = 1; i < n; i++){
    key = arr[i];
    j = i - 1;

    /* Move elements of arr[0..i-1], that are
    greater than key, to one position ahead
    of their current position */
    while (j >= 0 && arr[j] > key){
      arr[j + 1] = arr[j];
      j = j - 1;
    }
    arr[j + 1] = key;
  }
}

uint16_t average_Pixel(uint16_t arr[], int n) {  /* This function returns the mean value   */
    
    uint32_t sum = 0;
    for(int i= 0 ; i < n; i++){
        sum += arr[i];
    }
    return sum / n; 
}

/* Original Median Filter by Ullrich */
/* No boarder around the image       */
void FilterFrame(uint16_t (&frame)[NUMX][NUMY], uint16_t (&temp)[NUMX][NUMY]){
  uint16_t window[9];
  //Initialize a new array of same size of image with 0
  memset(temp, 0, sizeof(temp));

    for(byte row = 1; row < NUMY; row++) {
      for(byte col = 1; col < NUMX; col++) {
      //neighbor pixel values are stored in window including this pixel
      window[0] = frame[row-1][col-1];
      window[1] = frame[row-1][col];
      window[2] = frame[row-1][col+1];
      window[3] = frame[row][col-1];
      window[4] = frame[row][col];
      window[5] = frame[row][col+1];
      window[6] = frame[row+1][col-1];
      window[7] = frame[row+1][col];
      window[8] = frame[row+1][col+1];
      //sort window array
      insertionSort(window,9);
      //put the median to the new array
      temp[row][col]=window[4];
    }
  }
}

void filter_Frame (uint16_t (&frame)[NUMX][NUMY], uint16_t (&temp)[NUMX][NUMY]) {
  uint16_t window[9];
  //Initialize a new array of same size of image with 0
  uint16_t frame1 [NUMX][NUMY];
  memset(frame1,0,sizeof(frame1));
  schwell_wert_filter(frame,frame1);
  memset(temp, 0, sizeof(temp));
  
  for(uint16_t row = 1; row < NUMY; row++) {
    for(uint16_t col = 1; col < NUMX; col++) {
      //neighbor pixel values are stored in window including this pixel
    
      // wenn das Rand nicht NULL ist  und die For-Schleife von row = 1 bis 31 und col = 1 bis 31 
      window[0] = frame1[row-1][col-1];
      window[1] = frame1[row-1][col];
      window[2] = frame1[row-1][col+1];
      window[3] = frame1[row][col-1];
      window[4] = frame1[row][col];
      window[5] = frame1[row][col+1];
      window[6] = frame1[row+1][col-1];
      window[7] = frame1[row+1][col];
      window[8] = frame1[row+1][col+1];
     
//Serial.print( window[4]);
     
     //sort window array
      insertionSort(window,9);
                                                                                        
      //put the median to the new array
        temp [row][col]= window[4];                        // alternativ :temp[row][col]= (window[4]+window[3]+window[5]) / 3; 
        
      //temp [row][col]= average_Pixel(window, 9);
     // Serial.print(temp[row][col]);
      
    }
  }
}

void filter_Frame1 (uint16_t (&frame)[NUMX][NUMY], uint16_t (&temp)[NUMX][NUMY]) {
  uint16_t window[9];
  //Initialize a new array of same size of image with 0
  memset(temp, 0, sizeof(temp));
  uint16_t i = 0;
  for(uint16_t row = 0; row < NUMY; row++){       
    for(uint16_t col = 0; col < NUMX; col++){
      //Here the edges of the matrix are copied. 
      if(row == 0 && col == 0){   // Here the left corner of the matrix
        window[0] = 0;
        window[1] = 0;
        window[2] = 0;
        window[3] = 0;
        window[4] = frame[row][col];
        window[5] = frame[row][col+1];
        window[6] = 0;
        window[7] = frame[row+1][col];
        window[8] = frame[row+1][col+1];
      } else if (row == 0 ){  //Here the top of the matrix
        window[0] = 0;
        window[1] = 0;
        window[2] = 0;
        window[3] = frame[row][col];
        window[4] = frame[row][col+1];
        window[5] = frame[row][col+2];
        window[6] = frame[row+1][col];
        window[7] = frame[row+1][col+1];
        window[8] = frame[row+1][col+2]; 
      } else if (row == 31 && col == 0 ){  //Here the left side of the matrix below 
        window[0] = 0;
        window[1] = frame[row-1][col];
        window[2] = frame[row-1][col+1];
        window[3] = 0;
        window[4] = frame[row][col];
        window[5] = frame[row][col+1];
        window[6] = 0;
        window[7] = 0;
        window[8] = 0;      
      } else if (row == 31 ){
        window[0] = frame [row-1][col];
        window[1] = frame[row-1][col+1];
        window[2] = frame[row-1][col+2];
        window[3] = frame [row][col];
        window[4] = frame[row][col+1];
        window[5] = frame[row][col+2];
        window[6] = 0;
        window[7] = 0;
        window[8] = 0;
      } else if (row == 0 && col == 31){
        window[0] = 0;
        window[1] = 0;
        window[2] = 0;
        window[3] = frame[row][col-1];
        window[4] = frame[row][col];
        window[5] = 0;
        window[6] = frame[row+1][col-1];
        window[7] = frame[row+1][col];
        window[8] = 0; 
      } else if (row == 31 && col == 31){
        window[0] = frame [row-1][col-1];
        window[1] = frame [row-1][col];
        window[2] = 0;
        window[3] = frame [row][col-1];
        window[4] = frame [row][col];
        window[5] = 0;
        window[6] = 0;
        window[7] = 0;
        window[8] = 0;
      } else if(col == 0){
        window[0] = 0;
        window[1] = frame [row-1][col];
        window[2] = frame [row-1][col+1];
        window[3] = 0;
        window[4] = frame [row][col];
        window[5] = frame [row][col+1];
        window[6] = 0;
        window[7] = frame [row+1][col];
        window[8] = frame [row+1][col+1]; 
      } else if (col == 31){
        window[0] = frame [row-1][col-1];
        window[1] = frame[row-1][col];
        window[2] = 0;
        window[3] = frame [row][col-1];
        window[4] = frame[row][col];
        window[5] = 0;
        window[6] = frame[row+1][col-1];
        window[7] = frame[row+1][col];
        window[8] = 0;      
      } else { //Here is the window the matrix passes through
        window[0] = frame[row-1][col-1];
        window[1] = frame[row-1][col];
        window[2] = frame[row-1][col+1];
        window[3] = frame[row][col-1];
        window[4] = frame[row][col];
        window[5] = frame[row][col+1];
        window[6] = frame[row+1][col-1];
        window[7] = frame[row+1][col];
        window[8] = frame[row+1][col+1];
      }
  
     
     //sort window array
      insertionSort(window,9);
     
      
      //temp[row][col]= (window[4]+ window[3]+ window[5]) / 3; 
      //put the median to the new array  
      temp [row][col] = window[4]; 
    }
  }
}

void filter_Frame_Smoothing (uint16_t (&frame)[NUMX][NUMY], uint16_t (&temp)[NUMX][NUMY]) {
  uint16_t window[9];
  //Initialize a new array of same size of image with 0
  memset(temp, 0, sizeof(temp));
  uint16_t i = 0;
  for(uint16_t row = 0; row < NUMY; row++){       
    for(uint16_t col = 0; col < NUMX; col++){
    
      if(row == 0 && col == 0){     // Linken Ecke 
        window[0] = frame[row][col];
        window[1] = frame[row][col];
        window[2] = frame[row][col+1];
        window[3] = frame[row][col];
        window[4] = frame[row][col];
        window[5] = frame[row][col+1];
        window[6] = frame[row+1][col];
        window[7] = frame[row+1][col];
        window[8] = frame[row+1][col+1];
      } else if (row == 0 ){ //oben Seite
        window[0] = frame[row][col-1];
        window[1] = frame[row][col];
        window[2] = frame[row][col+1];
        window[3] = frame[row][col-1];
        window[4] = frame[row][col];
        window[5] = frame[row][col+1];
        window[6] = frame[row+1][col-1];
        window[7] = frame[row+1][col];
        window[8] = frame[row+1][col+1]; 
      }else if (row == 31 && col == 0 ){ // linke Ecke unten
        window[0] = frame[row-1][col];
        window[1] = frame[row-1][col];
        window[2] = frame[row-1][col+1];
        window[3] = frame[row][col];
        window[4] = frame[row][col];
        window[5] = frame[row][col+1];
        window[6] = frame[row][col];
        window[7] = frame[row][col];
        window[8] = frame[row][col+1];      
      }else if (row == 31 ){   //unten Seite
        window[0] = frame [row-1][col-1];
        window[1] = frame[row-1][col];
        window[2] = frame[row-1][col+1];
        window[3] = frame [row][col-1];
        window[4] = frame[row][col];
        window[5] = frame[row][col+1];
        window[6] = frame [row][col-1];
        window[7] = frame[row][col];
        window[8] = frame[row][col+1];
      }else if (row == 0 && col == 31){ // rechte Ecke Oben
        window[0] = frame[row][col-1];
        window[1] = frame[row][col];
        window[2] = frame[row][col];
        window[3] = frame[row][col-1];
        window[4] = frame[row][col];
        window[5] = frame[row][col];
        window[6] = frame[row+1][col-1];
        window[7] = frame[row+1][col];
        window[8] = frame[row+1][col]; 
      }else if (row == 31 && col == 31){ // rechte Ecke unten
        window[0] = frame [row-1][col-1];
        window[1] = frame [row-1][col];
        window[2] = frame [row-1][col];
        window[3] = frame [row][col-1];
        window[4] = frame [row][col];
        window[5] = frame [row][col];
        window[6] = frame [row][col-1];
        window[7] = frame [row][col];
        window[8] = frame [row][col]; 
      }else if(col == 0){ // linke seite 
        window[0] = frame [row-1][col];
        window[1] = frame [row-1][col];
        window[2] = frame [row-1][col+1];
        window[3] = frame [row][col];
        window[4] = frame [row][col];
        window[5] = frame [row][col+1];
        window[6] = frame [row+1][col];
        window[7] = frame [row+1][col];
        window[8] = frame [row+1][col+1];               
      }else if (col == 31){
        window[0] = frame [row-1][col-1];
        window[1] = frame[row-1][col];
        window[2] = frame[row-1][col];
        window[3] = frame [row][col-1];
        window[4] = frame[row][col];
        window[5] = frame[row][col];
        window[6] = frame[row+1][col-1];
        window[7] = frame[row+1][col];
        window[8] = frame[row+1][col];        
      }else {
        window[0] = frame[row-1][col-1];
        window[1] = frame[row-1][col];
        window[2] = frame[row-1][col+1];
        window[3] = frame[row][col-1];
        window[4] = frame[row][col];
        window[5] = frame[row][col+1];
        window[6] = frame[row+1][col-1];
        window[7] = frame[row+1][col];
        window[8] = frame[row+1][col+1];            
      }

      temp [row][col] = average_Pixel(window, 9);
      //Serial.print(temp [row][col]);
    }
  }
}


void schwell_wert_filter(uint16_t (&frame)[NUMX][NUMY], uint16_t (&temp)[NUMX][NUMY]) { //This is a function for the threshold filter
  
  for(uint16_t row = 0 ; row < NUMY; row++){       
    for(uint16_t col = 0; col < NUMX; col++){
      if(frame[row][col] < 2500){
        temp [row][col]= 0;   //The gray values that are smaller than the threshold are set to zero in result image.
      //}else if ( frame [row][col] > 2570 && frame[row][col] < 51400) {
        //temp [row][col]= (1.59375*frame [row][col])-15.9375;  // (170 > frame >10)
        //temp [row][col]= (1.3421*frame [row][col])-13.544;  // (180 > frame >10)
        //temp [row][col]= (frame[row][col]-)
      }else if (frame [row][col] > 2500){
        temp [row][col]= 4095; //The gray values that are greater than the threshold are set to 4095 in result image.
      }
    }
  }
}

void laplace_Schwell_Wert_Filter(uint16_t (&frame)[NUMX][NUMY], uint16_t (&temp)[NUMX][NUMY]) {//A function for threshold filter and Laplace operator filter 
  
  uint16_t window[9];  //Initialize a new array of same size of image with 0
  uint16_t schwell_Wert [NUMX][NUMY];  //Here a new 2d array NUMXY x NUMXY is instantized 
  /* Here are the convolution masks for the Laplace filter */
  int mask[9] = {0,-1,0,-1,4,-1,0,-1,0};          
  int mask1[9] ={1,-2,1,-2,4,-2,1,-2,1};
  int mask2[9] ={-1,-1,-1,-1,8,-1,-1,-1,-1};
  int mask3[9] ={1,1,1,1,-8,1,1,1,1};

  memset(schwell_Wert,0, sizeof(schwell_Wert));  //The array is filled with zeros.
  schwell_wert_filter(frame, schwell_Wert); //Here the function of the threshold filter is called.
  memset(temp, 0, sizeof(temp));  // The array is filled with zeros.

  for(uint16_t row = 1; row < 31; row++){
    for (uint16_t col = 1; col < 31; col++){
      
      window[0] = schwell_Wert[row-1][col-1];
      window[1] = schwell_Wert[row-1][col];
      window[2] = schwell_Wert[row-1][col+1];
      window[3] = schwell_Wert[row][col-1];
      window[4] = schwell_Wert[row][col];
      window[5] = schwell_Wert[row][col+1];
      window[6] = schwell_Wert[row+1][col-1];
      window[7] = schwell_Wert[row+1][col];
      window[8] = schwell_Wert[row+1][col+1];
     
      temp[row][col] = laplace_Gradient(window, mask, 9); //The laplace_Gradient function is called and filled into filtered image (temp) .
    }
    
  }
}
  
void imag(uint16_t (&frame)[NUMX][NUMY], uint16_t (&temp)[NUMX][NUMY]) { /*here the image is overwritten */
  memcpy(temp, frame, sizeof(frame));
  
  
  /*for (byte row = 0; row < NUMXY; row ++){
    for (byte col = 0; col < NUMX; col++){
      temp[row][col] += frame[row][col] + rand () % 5;    
    }
  }*/
}

uint16_t laplace_Gradient(uint16_t arr[], int mask[], int n){ /* In this function, the convolution for laplace filter between the mask and the distance window that passes through the original image */

  int sum = 0;
  for (uint16_t i = 0; i < n; i++){
    sum += (arr[i]*mask[i]);  /* here the folding is performed */  
  }

  if (sum > 4095){
    return sum = 4095;  
  } else if (sum < 0){
    return sum = 0;  
  }
  
 return sum; 
}
  
void laplace_filter(uint16_t (&frame)[NUMX][NUMY], uint16_t (&temp)[NUMX][NUMY]) { /* In this function the Laplace filter is performed */

  uint16_t window[9];

  /* Here are the convolution masks for the Laplace filter */
  
  int mask[9] = {0,-1,0,-1,4,-1,0,-1,0}; 
  int mask1[9] ={1,-2,1,-2,4,-2,1,-2,1};
  int mask2[9] ={-1,-1,-1,-1,8,-1,-1,-1,-1};
   memset (temp, 0, sizeof(temp));  // The array is filled with zeros.

    for (uint16_t row = 1; row < 31; row ++){
      for(uint16_t col = 1; col < 31; col ++){
        
        window[0] = frame[row-1][col-1];
        window[1] = frame[row-1][col];
        window[2] = frame[row-1][col+1];
        window[3] = frame[row][col-1];
        window[4] = frame[row][col];
        window[5] = frame[row][col+1];
        window[6] = frame[row+1][col-1];
        window[7] = frame[row+1][col];
        window[8] = frame[row+1][col+1];

        temp[row][col] = laplace_Gradient(window,mask,9); /* here the laplace_Gradient function is called and filled into filtered image (temp) */
  
      }
    }  
}

uint16_t sobel_Gradient(uint16_t arr [], int hx[], int hy[], uint8_t n ){ /*In this function, the convolution between the masks and the window that the image passes through */
 
  int sum = 0;    //A variable initialized 
  int sum1 = 0;   //A variable initialized 

  for (uint16_t i = 0 ; i < n; i++){
    sum  += (arr[i]*hx[i]);   //The convolution in direction x and summed sum
    sum1 += (arr[i]*hy[i]);   //The convolution in direction y and summed sum1
  }

  uint16_t b = sqrt(pow(sum, 2)+ pow(sum1,2));  //The amount of gradient calculated 

  if (b > 4095) {
    return b= 4095;
  }

  return b;
}
  
void sobel_Operator_Filter(uint16_t (&frame)[NUMX][NUMY], uint16_t (&temp)[NUMX][NUMY]) { /*Here the Sobel operator filter function is implemented*/

   uint16_t window [9];
   
   //int hx[9]= {-1,0,1,-1,0,1,-1,0,1}; /*Here is the convolution mask in X direction */
   //int hy[9]= {-1,-1,-1,0,0,0,1,1,1}; /*Here is the convolution mask in Y direction */
   int hy[9]= {1,0-1,2,0,-2,1,0,-1};
   int hx[9]= {1,2,1,0,0,0,-1,-2,-1};
   memset (temp, 0, sizeof(temp));

    for (uint16_t row = 1; row < 31; row ++){
      for(uint16_t col = 1; col < 31; col ++){
        
        window[0] = frame[row-1][col-1];
        window[1] = frame[row-1][col];
        window[2] = frame[row-1][col+1];
        window[3] = frame[row][col-1];
        window[4] = frame[row][col];
        window[5] = frame[row][col+1];
        window[6] = frame[row+1][col-1];
        window[7] = frame[row+1][col];
        window[8] = frame[row+1][col+1];

        temp[row][col] = sobel_Gradient(window, hx, hy, 9); /* here the sobel_Gradient function is called and filled into filtered image (temp) */
        
      }
    }
}

void laplace_Smoothing_Filter(uint16_t (&frame)[NUMX][NUMY], uint16_t (&temp)[NUMX][NUMY]) { /*a smoothing and Laplace filter function */
  
  uint16_t window[9];
  int mask[9]  ={0,-1,0,-1,4,-1,0,-1,0};          /* Here are the convolution masks for the Laplace filter */
  int mask1[9] ={1,-2,1,-2,4,-2,1,-2,1};
  int mask5[9] ={-1,2,-1,2,-4,2,-1,2,-1};
  int mask2[9] ={-1,-1,-1,-1,8,-1,-1,-1,-1};
  int mask3[9] ={1,1,1,1,-8,1,1,1,1};
  int mask4[9] ={0,1,0,1,-4,1,0,1,0};
  int hy[9]= {-1,-1,-1,0,0,0,1,1,1};
  int hx[9]= {-1,0,1,-1,0,1,-1,0,1};
  //int mask6[9] ={-0.5,-0.25,-0.25,-0.25,0,0.25,0.25,0.25,0.5}

  uint16_t smoothing[NUMX][NUMY];      /*Initialize a new array with the same size as the image with 0*/
  memset (smoothing, 0, sizeof(smoothing));
  //filter_Frame_Smoothing(frame, smoothing);   /*here the smoothing filter function is called */
  filter_Frame1(frame, smoothing);
  //Serial.print(smoothing[13][12]);
  memset (temp, 0, sizeof(temp));   /*fill the temp with 0 */
  for (uint16_t row = 1; row < 31; row ++){
      for(uint16_t col = 1; col < 31; col ++){
        
        window[0] = smoothing[row-1][col-1];
        window[1] = smoothing[row-1][col];
        window[2] = smoothing[row-1][col+1];
        window[3] = smoothing[row][col-1];
        window[4] = smoothing[row][col];
        window[5] = smoothing[row][col+1];
        window[6] = smoothing[row+1][col-1];
        window[7] = smoothing[row+1][col];
        window[8] = smoothing[row+1][col+1];

        temp[row][col] = laplace_Gradient(window,mask1, 9); /* here the laplace_Gradient function is called and filled into filtered image (temp) */
      }
    }
}

void Sobel_Smoothing (uint16_t (&frame)[NUMX][NUMY], uint16_t (&temp)[NUMX][NUMY]) { // Smoothig und Sobel-Operatorfilter function 

  uint16_t window [9];
   
  int hx[9]= {-1,0,1,-1,0,1,-1,0,1}; /*Here is the convolution mask in X direction */
  int hy[9]= {-1,-1,-1,0,0,0,1,1,1}; /*Here is the convolution mask in Y direction */

  uint16_t smoothing[NUMX][NUMY];      /*Initialize a new  2d array with the same size as the image with 0*/
  memset (smoothing, 0, sizeof(smoothing));
  filter_Frame_Smoothing(frame, smoothing); //Here the function of the threshold filter is called.
  memset (temp, 0, sizeof(temp));

  for (uint16_t row = 1; row < 31; row ++){
      for(uint16_t col = 1; col < 31; col ++){
        
        window[0] = smoothing[row-1][col-1];
        window[1] = smoothing[row-1][col];
        window[2] = smoothing[row-1][col+1];
        window[3] = smoothing[row][col-1];
        window[4] = smoothing[row][col];
        window[5] = smoothing[row][col+1];
        window[6] = smoothing[row+1][col-1];
        window[7] = smoothing[row+1][col];
        window[8] = smoothing[row+1][col+1];

        temp[row][col] = sobel_Gradient(window, hx, hy, 9); /* here the sobel_Gradient function is called and filled into filtered image (temp) */
      }
  }    
}


void smooting_schwell_Filter(uint16_t (&frame)[NUMX][NUMY], uint16_t (&temp)[NUMX][NUMY]) {// a smoothing and Schwellertfilter function
   

  uint16_t smoothing[NUMX][NUMY];      /*Initialize a new 2d array with the same size as the image with 0*/
  memset (smoothing, 0, sizeof(smoothing));
  filter_Frame_Smoothing(frame, smoothing);   /*here the smoothing filter function is called */
  //filter_Frame1(frame, smoothing);
  memset(temp,0, sizeof(temp));

  for(uint16_t row =0; row < NUMY; row++){
    for(uint16_t col =0; col < NUMX; col++){
      if(smoothing[row][col]< 2500){
        temp[row][col]= 0;
      }else if(smoothing[row][col]> 2500 && smoothing[row][col]< 3950){
       temp [row][col]= 4095;//(1.3421*frame [row][col])-13.544; 
      }else{ 
        temp[row][col] = 0;
      }
    }
  }
}

void Sobel_Smoothing_Schwellwert_Filter(uint16_t (&frame)[NUMX][NUMY], uint16_t (&temp)[NUMX][NUMY]) {  // Here we implement three filters one after the other.  

  uint16_t sobel_Smoothing[NUMX][NUMY];      /*Initialize a new array with the same size as the image with 0*/
  memset (sobel_Smoothing, 0, sizeof(sobel_Smoothing)); // here the Sobel_Smoothing function is called.
  Sobel_Smoothing(frame, sobel_Smoothing);
  //filter_Frame1(frame, smoothing);
  memset(temp,0, sizeof(temp));
  for(uint16_t row =0; row < NUMY; row++){
    for(uint16_t col =0; col < NUMX; col++){
      if(sobel_Smoothing[row][col]< 2800){
        temp[row][col]= 0;
      }else if(sobel_Smoothing[row][col]> 2800 && sobel_Smoothing[row][col]< 3950){
       temp [row][col]= 4095;//(1.3421*frame [row][col])-13.544; 
      }else{ 
        temp[row][col] = 0;
      }
    }
  }  


}
