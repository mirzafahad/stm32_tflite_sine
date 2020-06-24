/**
  ******************************************************************************
  * @file    lcd.h
  * @author  Fahad Mirza
  * @brief   Header file for LCD functionalities
  ******************************************************************************
  */

#ifndef LCD_H_
#define LCD_H_

/* Includes -----------------------------------------------------------------*/
#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"


/* Public variables ---------------------------------------------------------*/
// This constant represents the range of x values our model was trained on,
// which is from 0 to (2 * Pi). We approximate Pi to avoid requiring
// additional libraries.
const float INPUT_RANGE = 2.f * 3.14159265359f;


/* Public functions ---------------------------------------------------------*/
void LCD_Init(void);
// Called by the main loop to produce some output based on the x and y values
void LCD_Output(tflite::ErrorReporter* error_reporter, float x_value, float y_value);

#endif  // LCD_H_
