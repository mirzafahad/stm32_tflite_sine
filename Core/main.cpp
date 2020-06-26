/**
  ******************************************************************************
  * @file    main.cpp
  * @author  Fahad Mirza (fahadmirza8@gmail.com)
  * @brief   This file provides main program functions
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32746g_discovery.h"
#include "lcd.h"
#include "sine_model.h"
#include "tensorflow/lite/micro/kernels/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
namespace
{
    tflite::ErrorReporter* error_reporter = nullptr;
    const tflite::Model* model = nullptr;
    tflite::MicroInterpreter* interpreter = nullptr;
    TfLiteTensor* model_input = nullptr;
    TfLiteTensor* model_output = nullptr;

    // Create an area of memory to use for input, output, and intermediate arrays.
    // Finding the minimum value for your model may require some trial and error.
    constexpr uint32_t kTensorArenaSize = 2 * 1024;
    uint8_t tensor_arena[kTensorArenaSize];
} // namespace


// This constant represents the range of x values our model was trained on,
// which is from 0 to (2 * Pi). We approximate Pi to avoid requiring
// additional libraries.
extern const float INPUT_RANGE = 2.f * 3.14159265359f;
// NOTE: extern is used because lcd.c also uses this variable.

// This constant determines the number of inferences to perform across the range
// of x values defined above. Since each inference takes time, the higher this
// number, the more time it will take to run through the entire range. The value
// of this constant can be tuned so that one full cycle takes a desired amount
// of time. Since different devices take different amounts of time to perform
// inference, this value should be defined per-device.
// A larger number than the default to make the animation smoother
const uint16_t INFERENCE_PER_CYCLE = 70;

// UART handler declaration
UART_HandleTypeDef DebugUartHandler;


/* Private function prototypes -----------------------------------------------*/
static void system_clock_config(void);
static void cpu_cache_enable(void);
static void error_handler(void);
static void uart1_init(void);
void handle_output(tflite::ErrorReporter* error_reporter, float x_value, float y_value);


/* Private user code ---------------------------------------------------------*/
/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    // Enable the CPU Cache
	cpu_cache_enable();

    // Reset of all peripherals, Initializes the Flash interface and the Systick.
    HAL_Init();

    // Configure the system clock
    system_clock_config();

    // Configure on-board green LED
    BSP_LED_Init(LED_GREEN);

    // Initialize UART1
    uart1_init();

    // Initialize LCD
    LCD_Init();

  	static tflite::MicroErrorReporter micro_error_reporter;
  	error_reporter = &micro_error_reporter;

  	// Map the model into a usable data structure. This doesn't involve any
  	// copying or parsing, it's a very lightweight operation.
    model = tflite::GetModel(sine_model);

  	if(model->version() != TFLITE_SCHEMA_VERSION)
  	{
  		TF_LITE_REPORT_ERROR(error_reporter,
  	                         "Model provided is schema version %d not equal "
  	                         "to supported version %d.",
  	                         model->version(), TFLITE_SCHEMA_VERSION);
  	    return 0;
  	}

  	// This pulls in all the operation implementations we need.
  	static tflite::ops::micro::AllOpsResolver resolver;

  	// Build an interpreter to run the model with.
  	static tflite::MicroInterpreter static_interpreter(model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
  	interpreter = &static_interpreter;

  	// Allocate memory from the tensor_arena for the model's tensors.
  	TfLiteStatus allocate_status = interpreter->AllocateTensors();
  	if (allocate_status != kTfLiteOk)
  	{
  	    TF_LITE_REPORT_ERROR(error_reporter, "AllocateTensors() failed");
  	    return 0;
  	}

  	// Obtain pointers to the model's input and output tensors.
  	model_input = interpreter->input(0);
  	model_output = interpreter->output(0);

    // We are dividing the whole input range with the number of inference
    // per cycle we want to show to get the unit value. We will then multiply
    // the unit value with the current position of the inference
    float unitValuePerDevision = INPUT_RANGE / static_cast<float>(INFERENCE_PER_CYCLE);
 
    while (1)
    {
	    // Calculate an x value to feed into the model
        for(uint16_t inferenceCount = 0; inferenceCount <= INFERENCE_PER_CYCLE; inferenceCount++)
        {
	        float x_val = static_cast<float>(inferenceCount) * unitValuePerDevision;

	        // Place our calculated x value in the model's input tensor
	        model_input->data.f[0] = x_val;

	        // Run inference, and report any error
	        TfLiteStatus invoke_status = interpreter->Invoke();
	        if (invoke_status != kTfLiteOk)
	        {
	            TF_LITE_REPORT_ERROR(error_reporter, "Invoke failed on x_val: %f\n", static_cast<float>(x_val));
	            return 0;
	        }

	        // Read the predicted y value from the model's output tensor
	        float y_val = model_output->data.f[0];

	        // Do something with the results
	        handle_output(error_reporter, x_val, y_val);
        }
    }
}


void handle_output(tflite::ErrorReporter* error_reporter, float x_value, float y_value)
{
	// Log the current X and Y values
	TF_LITE_REPORT_ERROR(error_reporter, "x_value: %f, y_value: %f\n", x_value, y_value);

	// A custom function can be implemented and used here to do something with the x and y values.
	// In my case I will be plotting sine wave on an LCD.
	LCD_Output(x_value, y_value);
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 200000000
  *            HCLK(Hz)                       = 200000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 25000000
  *            PLL_M                          = 25
  *            PLL_N                          = 400
  *            PLL_P                          = 2
  *            PLL_Q                          = 9
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 6
  * @param  None
  * @retval None
  */
void system_clock_config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};


    // Configure the main internal regulator output voltage
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    // Enable HSE Oscillator and activate PLL with HSE as source
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM       = 25;
    RCC_OscInitStruct.PLL.PLLN       = 400;
    RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ       = 9;

    if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
    	error_handler();
    }

    // Activate the Over-Drive mode
    if(HAL_PWREx_EnableOverDrive() != HAL_OK)
    {
    	error_handler();
    }

    // Initializes the CPU, AHB and APB busses clocks
    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_6) != HAL_OK)
    {
        error_handler();
    }
}


/**
  * @brief  UART1 Initialization Function
  * @param  None
  * @retval None
  */
static void uart1_init(void)
{
    /*##-1- Configure the UART peripheral ######################################*/
	/* Put the USART peripheral in the Asynchronous mode (UART Mode)
	   UART configured as follows:
	      - Word Length = 8 Bits
	      - Stop Bit = One Stop bit
	      - Parity = None
	      - BaudRate = 9600 baud
	      - Hardware flow control disabled (RTS and CTS signals)
	 */

	DebugUartHandler.Instance        = DISCOVERY_COM1;
	DebugUartHandler.Init.BaudRate   = 9600;
	DebugUartHandler.Init.WordLength = UART_WORDLENGTH_8B;
	DebugUartHandler.Init.StopBits   = UART_STOPBITS_1;
	DebugUartHandler.Init.Parity     = UART_PARITY_NONE;
	DebugUartHandler.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
	DebugUartHandler.Init.Mode       = UART_MODE_TX_RX;
	DebugUartHandler.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

	if(HAL_UART_Init(&DebugUartHandler) != HAL_OK)
	{
	    error_handler();
	}
}


/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void error_handler(void)
{
    // Turn Green LED ON
    BSP_LED_On(LED_GREEN);
    while(1);
}

/**
  * @brief  CPU L1-Cache enable.
  * @param  None
  * @retval None
  */
static void cpu_cache_enable(void)
{
    // Enable I-Cache
    SCB_EnableICache();

    // Enable D-Cache
    SCB_EnableDCache();
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
}
#endif // USE_FULL_ASSERT
