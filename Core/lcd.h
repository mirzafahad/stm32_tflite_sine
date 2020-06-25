/**
  ******************************************************************************
  * @file    lcd.h
  * @author  Fahad Mirza
  * @brief   Header file for LCD functionalities
  ******************************************************************************
  */

#ifndef LCD_H_
#define LCD_H_

#ifdef __cplusplus
 extern "C" {
#endif

/* Public variables ---------------------------------------------------------*/
// This constant represents the range of x values our model was trained on,
// which is from 0 to (2 * Pi). We approximate Pi to avoid requiring
// additional libraries.
const float INPUT_RANGE = 2.f * 3.14159265359f;


/* Public functions ---------------------------------------------------------*/
void LCD_Init(void);
// Called by the main loop handle_output() to plot y vs x
void LCD_Output(float x_value, float y_value);

#ifdef __cplusplus
}
#endif

#endif  // LCD_H_
