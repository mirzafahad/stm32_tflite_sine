/**
  ******************************************************************************
  * @file    lcd.c
  * @author  Fahad Mirza
  * @brief   This file provides LCD related functionalities
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "lcd.h"
#include "stm32746g_discovery_lcd.h"

/* Private variables ---------------------------------------------------------*/
// The colors we'll draw
static const uint32_t BACKGROUND_COLOR = LCD_COLOR_YELLOW;
static const uint32_t FOREGROUND_COLOR = LCD_COLOR_RED;
// The size of the dot we'll draw
static const uint8_t DOT_RADIUS = 10;

// Size of the drawable area
static uint16_t LcdWidth;
static uint16_t LcdHeight;

// Midpoint of the y axis
static uint16_t Midpoint;

// Pixels per unit of x_value
static float X_increment;


/* Function definitions -----------------------------------------------------*/
/**
  * @brief  LCD and global variables Initialization
  * @param  None
  * @retval None
  */
void LCD_Init(void)
{
    // LCD Initialization
    BSP_LCD_Init();

    // LCD Initialization
    BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);
    BSP_LCD_LayerDefaultInit(1, LCD_FB_START_ADDRESS+(BSP_LCD_GetXSize()*BSP_LCD_GetYSize()*4));

    // Enable the LCD
    BSP_LCD_DisplayOn();

    // Select the LCD Background Layer
    BSP_LCD_SelectLayer(0);

    // Clear the Background Layer
    BSP_LCD_Clear(BACKGROUND_COLOR);

    // Select the LCD Foreground Layer
    BSP_LCD_SelectLayer(1);

    // Clear the Foreground Layer
    BSP_LCD_Clear(FOREGROUND_COLOR);

    // Set the foreground color (Setting text color will set the foreground color)
    BSP_LCD_SetTextColor(FOREGROUND_COLOR);

    // Configure the transparency for foreground and background: Increase the transparency
    BSP_LCD_SetTransparency(0, 0);
    BSP_LCD_SetTransparency(1, 100);

    // Calculate the drawable area to avoid drawing off the edges
    LcdWidth = BSP_LCD_GetXSize() - ((DOT_RADIUS * 2) * 2);  // Taking off space equal to diameter
    LcdHeight = BSP_LCD_GetYSize() - ((DOT_RADIUS * 2) * 2); // of the dot from both sides

    // Calculate the y axis midpoint
    Midpoint = LcdHeight / 2;

    // Calculate fractional pixels per unit of x_value
    X_increment = static_cast<float>(LcdWidth) / INPUT_RANGE;
}

/**
  * @brief  LCD Output for the NN inference
  *         Animates a dot across the screen to represent the current x and y values
  * @param  None
  * @retval None
  */
void LCD_Output(tflite::ErrorReporter* error_reporter, float x_value, float y_value)
{
    // Log the current X and Y values
    TF_LITE_REPORT_ERROR(error_reporter, "x_value: %f, y_value: %f\n", x_value, y_value);

    if(x_value == 0)
    {
        // Clear the previous drawing, we are starting from the beginning
        BSP_LCD_Clear(BACKGROUND_COLOR);
    }

    // Calculate x position, ensuring the dot is not partially offscreen,
    // which causes artifacts and crashes
    uint32_t x_pos = (DOT_RADIUS * 2) + static_cast<uint32_t>(x_value * X_increment);

    // Calculate y position, ensuring the dot is not partially offscreen
    uint32_t y_pos;
    if(y_value >= 0)
    {
        // Since the display's y runs from the top down, invert y_value
        y_pos = (DOT_RADIUS * 2) + static_cast<uint32_t>(Midpoint * (1.f - y_value));

        // NOTE: We took off the diameter of the dot from the actual LCD space,
        //       when we calculated LcdHeight. So, add that to the actual y_pos
        //       to get the correct y_pos.
    }
    else
    {
        // For any negative y_value, start drawing from the midpoint
        y_pos = (DOT_RADIUS * 2) + Midpoint + static_cast<uint32_t>(Midpoint * (0.f - y_value));

        // NOTE: We took off the diameter of the dot from the actual LCD space,
        //       when we calculated LcdHeight. So, add that to the actual y_pos
        //       to get the correct y_pos.
    }

    // Draw the dot
    BSP_LCD_FillCircle(x_pos, y_pos, DOT_RADIUS);
}
