/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "rng.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "string.h"
#include "stdio.h"
#include "ILI9341_Touchscreen.h"

#include "ILI9341_STM32_Driver.h"
#include "ILI9341_GFX.h"

#include "snow_tiger.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
float h=30,t=40.0;
uint8_t step=0;
HAL_StatusTypeDef status;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* USER CODE BEGIN 4 */
//uint16_t CRC16_2(uint8_t*, uint8_t);
/* USER CODE BEGIN 4 */
uint16_t CRC16_2(uint8_t *ptr, uint8_t length)
{
    uint16_t crc = 0xFFFF;
    uint8_t  s   = 0x00;

    while(length--) {
        crc ^= *ptr++;
        for(s = 0; s < 8; s++) {
            if((crc & 0x01) != 0) {
                crc >>= 1;
                crc ^= 0xA001;
            } else crc >>= 1;
        }
    }
    return crc;
}
/* USER CODE END 4 */



/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// ประกาศตัวแปร
uint8_t AM2320_ADDR = 0x5C << 1; // 0xB8
uint8_t cmdBuffer[3];
uint8_t cmdBuffer[3] = {0x03, 0x00, 0x04};
char str[50];
uint8_t dataBuffer[8];

 // คำสั่งอ่าน, รีจิสเตอร์เริ่มต้น, จำนวนไบต์
uint8_t rxBuffer[8];
float temperature = 0.0;
float humidity = 0.0;
int flag = 0,flag2 = 0;
// ฟังก์ชันสำหรับอ่านค่า

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* Enable the CPU Cache */

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART3_UART_Init();
  MX_RNG_Init();
  MX_SPI5_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
  char charbuf[100];

  char uart_buf[50];
    sprintf(uart_buf, "\r\n--- Starting I2C Scan ---\r\n");
    HAL_UART_Transmit(&huart3, (uint8_t *)uart_buf, strlen(uart_buf), 100);

    // วนลูปเช็ค Address ตั้งแต่ 1 ถึง 127
    for (uint8_t i = 1; i < 128; i++)
    {
        // HAL ต้องการ Address แบบ 8 บิต จึงต้องเอา 7 บิตมา Shift left 1 ตำแหน่ง (i << 1)
        HAL_StatusTypeDef result = HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(i << 1), 3, 100);

        if (result == HAL_OK) // ถ้าเซนเซอร์ตอบกลับมา
        {
            sprintf(uart_buf, "Found I2C device at Address: 0x%02X\r\n", i);
            HAL_UART_Transmit(&huart3, (uint8_t *)uart_buf, strlen(uart_buf), 100);
        }
    }

    sprintf(uart_buf, "--- Scan Complete ---\r\n\r\n");
    HAL_UART_Transmit(&huart3, (uint8_t *)uart_buf, strlen(uart_buf), 100);

    cmdBuffer[0] = 0x03;
    cmdBuffer[1] = 0x00;
    cmdBuffer[2] = 0x04;
    void Read_AM2320(void) {
        // 1. ส่งสัญญาณปลุกเซนเซอร์ (ฟังก์ชันนี้อาจรีเทิร์น HAL_ERROR ซึ่งถือว่าปกติในขั้นตอนนี้)
        HAL_I2C_Master_Transmit(&hi2c1, AM2320_ADDR, NULL, 0, 100);
        HAL_Delay(1); // รอเซนเซอร์ตื่น

        // 2. ส่งคำสั่งขออ่านข้อมูล
        if (HAL_I2C_Master_Transmit(&hi2c1, AM2320_ADDR, cmdBuffer, 3, 100) == HAL_OK) {

            HAL_Delay(2); // รอเซนเซอร์ประมวลผลตามสเปค
            flag++;
            // 3. อ่านข้อมูลกลับมา 8 ไบต์
            if (HAL_I2C_Master_Receive(&hi2c1, AM2320_ADDR, rxBuffer, 8, 100) == HAL_OK) {
            	flag2++;
                // ข้อมูลความชื้นอยู่ไบต์ที่ 2 และ 3
                uint16_t raw_humidity = (rxBuffer[2] << 8) | rxBuffer[3];
                humidity = raw_humidity / 10.0f;

                // ข้อมูลอุณหภูมิอยู่ไบต์ที่ 4 และ 5
                uint16_t raw_temp = (rxBuffer[4] << 8) | rxBuffer[5];

                // เช็คบิตเครื่องหมาย (กรณีติดลบ)
                if (raw_temp & 0x8000) {
                    temperature = -(raw_temp & 0x7FFF) / 10.0f;
                } else {
                    temperature = raw_temp / 10.0f;
                }
            }
        }
    }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    while (1)
      {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    //----------------------------------------------------------PERFORMANCE TEST
    		ILI9341_Fill_Screen(WHITE);
    		ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
    		ILI9341_Draw_Text("FPS TEST, 40 loop 2 screens", 10, 10, BLACK, 1, WHITE);
    		HAL_Delay(2000);
    		ILI9341_Fill_Screen(WHITE);

    		uint32_t Timer_Counter = 0;
    		for(uint32_t j = 0; j < 2; j++)
    		{
    			HAL_TIM_Base_Start(&htim1);
    			for(uint16_t i = 0; i < 10; i++)
    			{
    				ILI9341_Fill_Screen(WHITE);
    				ILI9341_Fill_Screen(BLACK);
    			}

    			//20.000 per second!
    			HAL_TIM_Base_Stop(&htim1);
    			Timer_Counter += __HAL_TIM_GET_COUNTER(&htim1);
    			__HAL_TIM_SET_COUNTER(&htim1, 0);
    		}
    		Timer_Counter /= 2;

    		char counter_buff[30];
    		ILI9341_Fill_Screen(WHITE);
    		ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
    		sprintf(counter_buff, "Timer counter value: %d", Timer_Counter*2);
    		ILI9341_Draw_Text(counter_buff, 10, 10, BLACK, 1, WHITE);

    		double seconds_passed = 2*((float)Timer_Counter / 20000);
    		sprintf(counter_buff, "Time: %.3f Sec", seconds_passed);
    		ILI9341_Draw_Text(counter_buff, 10, 30, BLACK, 2, WHITE);

    		double timer_float = 20/(((float)Timer_Counter)/20000);	//Frames per sec

    		sprintf(counter_buff, "FPS:  %.2f", timer_float);
    		ILI9341_Draw_Text(counter_buff, 10, 50, BLACK, 2, WHITE);
    		double MB_PS = timer_float*240*320*2/1000000;
    		sprintf(counter_buff, "MB/S: %.2f", MB_PS);
    		ILI9341_Draw_Text(counter_buff, 10, 70, BLACK, 2, WHITE);
    		double SPI_utilized_percentage = (MB_PS/(6.25 ))*100;		//50mbits / 8 bits
    		sprintf(counter_buff, "SPI Utilized: %.2f", SPI_utilized_percentage);
    		ILI9341_Draw_Text(counter_buff, 10, 90, BLACK, 2, WHITE);
    		HAL_Delay(10000);


    		static uint16_t x = 0;
    		static uint16_t y = 0;

    		char Temp_Buffer_text[40];

    //----------------------------------------------------------COUNTING MULTIPLE SEGMENTS
    		ILI9341_Fill_Screen(WHITE);
    		ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
    		ILI9341_Draw_Text("Counting multiple segments at once", 10, 10, BLACK, 1, WHITE);
    		HAL_Delay(2000);
    		ILI9341_Fill_Screen(WHITE);


    		for(uint16_t i = 0; i <= 10; i++)
    		{
    		sprintf(Temp_Buffer_text, "Counting: %d", i);
    		ILI9341_Draw_Text(Temp_Buffer_text, 10, 10, BLACK, 2, WHITE);
    		ILI9341_Draw_Text(Temp_Buffer_text, 10, 30, BLUE, 2, WHITE);
    		ILI9341_Draw_Text(Temp_Buffer_text, 10, 50, RED, 2, WHITE);
    		ILI9341_Draw_Text(Temp_Buffer_text, 10, 70, GREEN, 2, WHITE);
    		ILI9341_Draw_Text(Temp_Buffer_text, 10, 90, BLACK, 2, WHITE);
    		ILI9341_Draw_Text(Temp_Buffer_text, 10, 110, BLUE, 2, WHITE);
    		ILI9341_Draw_Text(Temp_Buffer_text, 10, 130, RED, 2, WHITE);
    		ILI9341_Draw_Text(Temp_Buffer_text, 10, 150, GREEN, 2, WHITE);
    		ILI9341_Draw_Text(Temp_Buffer_text, 10, 170, WHITE, 2, BLACK);
    		ILI9341_Draw_Text(Temp_Buffer_text, 10, 190, BLUE, 2, BLACK);
    		ILI9341_Draw_Text(Temp_Buffer_text, 10, 210, RED, 2, BLACK);
    		}

    		HAL_Delay(1000);

    //----------------------------------------------------------COUNTING SINGLE SEGMENT
    		ILI9341_Fill_Screen(WHITE);
    		ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
    		ILI9341_Draw_Text("Counting single segment", 10, 10, BLACK, 1, WHITE);
    		HAL_Delay(2000);
    		ILI9341_Fill_Screen(WHITE);

    		for(uint16_t i = 0; i <= 100; i++)
    		{
    		sprintf(Temp_Buffer_text, "Counting: %d", i);
    		ILI9341_Draw_Text(Temp_Buffer_text, 10, 10, BLACK, 3, WHITE);
    		}

    		HAL_Delay(1000);

    //----------------------------------------------------------ALIGNMENT TEST
    		ILI9341_Fill_Screen(WHITE);
    		ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
    		ILI9341_Draw_Text("Rectangle alignment check", 10, 10, BLACK, 1, WHITE);
    		HAL_Delay(2000);
    		ILI9341_Fill_Screen(WHITE);

    		ILI9341_Draw_Hollow_Rectangle_Coord(50, 50, 100, 100, BLACK);
    		ILI9341_Draw_Filled_Rectangle_Coord(20, 20, 50, 50, BLACK);
    		ILI9341_Draw_Hollow_Rectangle_Coord(10, 10, 19, 19, BLACK);
    		HAL_Delay(1000);

    //----------------------------------------------------------LINES EXAMPLE
    		ILI9341_Fill_Screen(WHITE);
    		ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
    		ILI9341_Draw_Text("Randomly placed and sized", 10, 10, BLACK, 1, WHITE);
    		ILI9341_Draw_Text("Horizontal and Vertical lines", 10, 20, BLACK, 1, WHITE);
    		HAL_Delay(2000);
    		ILI9341_Fill_Screen(WHITE);

    		for(uint32_t i = 0; i < 30000; i++)
    		{
    			uint32_t random_num = 0;
    			uint16_t xr = 0;
    			uint16_t yr = 0;
    			uint16_t radiusr = 0;
    			uint16_t colourr = 0;
    			random_num = HAL_RNG_GetRandomNumber(&hrng);
    			xr = random_num;
    			random_num = HAL_RNG_GetRandomNumber(&hrng);
    			yr = random_num;
    			random_num = HAL_RNG_GetRandomNumber(&hrng);
    			radiusr = random_num;
    			random_num = HAL_RNG_GetRandomNumber(&hrng);
    			colourr = random_num;

    			xr &= 0x01FF;
    			yr &= 0x01FF;
    			radiusr &= 0x001F;
    			//ili9341_drawpixel(xr, yr, WHITE);
    			ILI9341_Draw_Horizontal_Line(xr, yr, radiusr, colourr);
    			ILI9341_Draw_Vertical_Line(xr, yr, radiusr, colourr);
    		}

    		HAL_Delay(1000);

    //----------------------------------------------------------HOLLOW CIRCLES EXAMPLE
    		ILI9341_Fill_Screen(WHITE);
    		ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
    		ILI9341_Draw_Text("Randomly placed and sized", 10, 10, BLACK, 1, WHITE);
    		ILI9341_Draw_Text("Circles", 10, 20, BLACK, 1, WHITE);
    		HAL_Delay(2000);
    		ILI9341_Fill_Screen(WHITE);


    		for(uint32_t i = 0; i < 3000; i++)
    		{
    			uint32_t random_num = 0;
    			uint16_t xr = 0;
    			uint16_t yr = 0;
    			uint16_t radiusr = 0;
    			uint16_t colourr = 0;
    			random_num = HAL_RNG_GetRandomNumber(&hrng);
    			xr = random_num;
    			random_num = HAL_RNG_GetRandomNumber(&hrng);
    			yr = random_num;
    			random_num = HAL_RNG_GetRandomNumber(&hrng);
    			radiusr = random_num;
    			random_num = HAL_RNG_GetRandomNumber(&hrng);
    			colourr = random_num;

    			xr &= 0x01FF;
    			yr &= 0x01FF;
    			radiusr &= 0x001F;
    			//ili9341_drawpixel(xr, yr, WHITE);
    			ILI9341_Draw_Hollow_Circle(xr, yr, radiusr*2, colourr);
    		}
    		HAL_Delay(1000);

    //----------------------------------------------------------FILLED CIRCLES EXAMPLE
    		ILI9341_Fill_Screen(WHITE);
    		ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
    		ILI9341_Draw_Text("Randomly placed and sized", 10, 10, BLACK, 1, WHITE);
    		ILI9341_Draw_Text("Filled Circles", 10, 20, BLACK, 1, WHITE);
    		HAL_Delay(2000);
    		ILI9341_Fill_Screen(WHITE);

    		for(uint32_t i = 0; i < 1000; i++)
    		{
    			uint32_t random_num = 0;
    			uint16_t xr = 0;
    			uint16_t yr = 0;
    			uint16_t radiusr = 0;
    			uint16_t colourr = 0;
    			random_num = HAL_RNG_GetRandomNumber(&hrng);
    			xr = random_num;
    			random_num = HAL_RNG_GetRandomNumber(&hrng);
    			yr = random_num;
    			random_num = HAL_RNG_GetRandomNumber(&hrng);
    			radiusr = random_num;
    			random_num = HAL_RNG_GetRandomNumber(&hrng);
    			colourr = random_num;

    			xr &= 0x01FF;
    			yr &= 0x01FF;
    			radiusr &= 0x001F;
    			//ili9341_drawpixel(xr, yr, WHITE);
    			ILI9341_Draw_Filled_Circle(xr, yr, radiusr/2, colourr);
    		}
    		HAL_Delay(1000);

    //----------------------------------------------------------HOLLOW RECTANGLES EXAMPLE
    		ILI9341_Fill_Screen(WHITE);
    		ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
    		ILI9341_Draw_Text("Randomly placed and sized", 10, 10, BLACK, 1, WHITE);
    		ILI9341_Draw_Text("Rectangles", 10, 20, BLACK, 1, WHITE);
    		HAL_Delay(2000);
    		ILI9341_Fill_Screen(WHITE);

    		for(uint32_t i = 0; i < 20000; i++)
    		{
    			uint32_t random_num = 0;
    			uint16_t xr = 0;
    			uint16_t yr = 0;
    			uint16_t radiusr = 0;
    			uint16_t colourr = 0;
    			random_num = HAL_RNG_GetRandomNumber(&hrng);
    			xr = random_num;
    			random_num = HAL_RNG_GetRandomNumber(&hrng);
    			yr = random_num;
    			random_num = HAL_RNG_GetRandomNumber(&hrng);
    			radiusr = random_num;
    			random_num = HAL_RNG_GetRandomNumber(&hrng);
    			colourr = random_num;

    			xr &= 0x01FF;
    			yr &= 0x01FF;
    			radiusr &= 0x001F;
    			//ili9341_drawpixel(xr, yr, WHITE);
    			ILI9341_Draw_Hollow_Rectangle_Coord(xr, yr, xr+radiusr, yr+radiusr, colourr);
    		}
    		HAL_Delay(1000);

    //----------------------------------------------------------FILLED RECTANGLES EXAMPLE
    		ILI9341_Fill_Screen(WHITE);
    		ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
    		ILI9341_Draw_Text("Randomly placed and sized", 10, 10, BLACK, 1, WHITE);
    		ILI9341_Draw_Text("Filled Rectangles", 10, 20, BLACK, 1, WHITE);
    		HAL_Delay(2000);
    		ILI9341_Fill_Screen(WHITE);

    		for(uint32_t i = 0; i < 20000; i++)
    		{
    			uint32_t random_num = 0;
    			uint16_t xr = 0;
    			uint16_t yr = 0;
    			uint16_t radiusr = 0;
    			uint16_t colourr = 0;
    			random_num = HAL_RNG_GetRandomNumber(&hrng);
    			xr = random_num;
    			random_num = HAL_RNG_GetRandomNumber(&hrng);
    			yr = random_num;
    			random_num = HAL_RNG_GetRandomNumber(&hrng);
    			radiusr = random_num;
    			random_num = HAL_RNG_GetRandomNumber(&hrng);
    			colourr = random_num;

    			xr &= 0x01FF;
    			yr &= 0x01FF;
    			radiusr &= 0x001F;
    			//ili9341_drawpixel(xr, yr, WHITE);
    			ILI9341_Draw_Rectangle(xr, yr, radiusr, radiusr, colourr);
    		}
    		HAL_Delay(1000);

    //----------------------------------------------------------INDIVIDUAL PIXEL EXAMPLE

    		ILI9341_Fill_Screen(WHITE);
    		ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
    		ILI9341_Draw_Text("Slow draw by selecting", 10, 10, BLACK, 1, WHITE);
    		ILI9341_Draw_Text("and adressing pixels", 10, 20, BLACK, 1, WHITE);
    		HAL_Delay(2000);
    		ILI9341_Fill_Screen(WHITE);


    		x = 0;
    		y = 0;
    		while (y < 240)
    		{
    		while ((x < 320) && (y < 240))
    		{

    			if(x % 2)
    			{
    				ILI9341_Draw_Pixel(x, y, BLACK);
    			}

    			x++;
    		}

    			y++;
    			x = 0;
    		}

    		x = 0;
    		y = 0;


    		while (y < 240)
    		{
    		while ((x < 320) && (y < 240))
    		{

    			if(y % 2)
    			{
    				ILI9341_Draw_Pixel(x, y, BLACK);
    			}

    			x++;
    		}

    			y++;
    			x = 0;
    		}
    		HAL_Delay(2000);

    //----------------------------------------------------------INDIVIDUAL PIXEL EXAMPLE
    		ILI9341_Fill_Screen(WHITE);
    		ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
    		ILI9341_Draw_Text("Random position and colour", 10, 10, BLACK, 1, WHITE);
    		ILI9341_Draw_Text("500000 pixels", 10, 20, BLACK, 1, WHITE);
    		HAL_Delay(2000);
    		ILI9341_Fill_Screen(WHITE);


    		for(uint32_t i = 0; i < 500000; i++)
    		{
    			uint32_t random_num = 0;
    			uint16_t xr = 0;
    			uint16_t yr = 0;
    			random_num = HAL_RNG_GetRandomNumber(&hrng);
    			xr = random_num;
    			random_num = HAL_RNG_GetRandomNumber(&hrng);
    			yr = random_num;
    			uint16_t color = HAL_RNG_GetRandomNumber(&hrng);

    			xr &= 0x01FF;
    			yr &= 0x01FF;
    			ILI9341_Draw_Pixel(xr, yr, color);
    		}
    		HAL_Delay(2000);

    //----------------------------------------------------------565 COLOUR EXAMPLE, Grayscale
    		ILI9341_Fill_Screen(WHITE);
    		ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
    		ILI9341_Draw_Text("Colour gradient", 10, 10, BLACK, 1, WHITE);
    		ILI9341_Draw_Text("Grayscale", 10, 20, BLACK, 1, WHITE);
    		HAL_Delay(2000);


    		for(uint16_t i = 0; i <= (320); i++)
    		{
    			uint16_t Red = 0;
    			uint16_t Green = 0;
    			uint16_t Blue = 0;

    			Red = i/(10);
    			Red <<= 11;
    			Green = i/(5);
    			Green <<= 5;
    			Blue = i/(10);



    			uint16_t RGB_color = Red + Green + Blue;
    			ILI9341_Draw_Rectangle(i, x, 1, 240, RGB_color);

    		}
    		HAL_Delay(2000);

    //----------------------------------------------------------IMAGE EXAMPLE, Snow Tiger
    		ILI9341_Fill_Screen(WHITE);
    		ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
    		ILI9341_Draw_Text("RGB Picture", 10, 10, BLACK, 1, WHITE);
    		ILI9341_Draw_Text("TIGER", 10, 20, BLACK, 1, WHITE);
    		HAL_Delay(2000);
    		ILI9341_Draw_Image((const char*)snow_tiger, SCREEN_VERTICAL_2);
    		ILI9341_Set_Rotation(SCREEN_VERTICAL_1);
    		HAL_Delay(10000);


    //----------------------------------------------------------TOUCHSCREEN EXAMPLE
    		ILI9341_Fill_Screen(WHITE);
    		ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
    		ILI9341_Draw_Text("Touchscreen", 10, 10, BLACK, 2, WHITE);
    		ILI9341_Draw_Text("Touch to draw", 10, 30, BLACK, 2, WHITE);
    		ILI9341_Set_Rotation(SCREEN_VERTICAL_1);

    		while(1)
    		{

    			if(TP_Touchpad_Pressed())
            {

    					uint16_t x_pos = 0;
    					uint16_t y_pos = 0;


    					HAL_GPIO_WritePin(GPIOB, LD3_Pin|LD2_Pin, GPIO_PIN_SET);

              uint16_t position_array[2];

    					if(TP_Read_Coordinates(position_array) == TOUCHPAD_DATA_OK)
    					{
    					x_pos = position_array[0];
    					y_pos = position_array[1];
    					ILI9341_Draw_Filled_Circle(x_pos, y_pos, 2, BLACK);

    					ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
    					char counter_buff[30];
    					sprintf(counter_buff, "POS X: %.3d", x_pos);
    					ILI9341_Draw_Text(counter_buff, 10, 80, BLACK, 2, WHITE);
    					sprintf(counter_buff, "POS Y: %.3d", y_pos);
    					ILI9341_Draw_Text(counter_buff, 10, 120, BLACK, 2, WHITE);
    					ILI9341_Set_Rotation(SCREEN_VERTICAL_1);
    					}

    					//ILI9341_Draw_Pixel(x_pos, y_pos, BLACK);

            }
    			else
    			{
    				HAL_GPIO_WritePin(GPIOB, LD3_Pin|LD2_Pin, GPIO_PIN_RESET);
    			}

    		}


      }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 12;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 9;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_6) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
//uint16_t CRC16_2(uint8_t*, uint8_t);
/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
