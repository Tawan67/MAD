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
#include "stdpic.h"

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
uint16_t CRC16_2(uint8_t *ptr, uint8_t length);

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
char ctemper[10];
char chumid[10];
 // คำสั่งอ่าน, รีจิสเตอร์เริ่มต้น, จำนวนไบต์
uint8_t rxBuffer[8];
float temperature = 0.0;
float humidity = 0.0;
int flag = 0,flag2 = 0;
uint8_t red_bar = 0,green_bar = 0,blue_bar = 0;
int flagDisplay = 1;
uint32_t now;
uint32_t check;
// ฟังก์ชันสำหรับอ่านค่า
uint8_t r=0,g=0,b=0;
uint16_t RGB565_CONVERT(uint8_t R, uint8_t G, uint8_t B)
{
    // การแปลงค่า:
    // 1. ลดทอนค่า 8-bit (0-255) ให้เหลือตามจำนวนบิตของแต่ละสี (5 หรือ 6 bit)
    //    โดยการ shift right (R>>3, G>>2, B>>3)
    // 2. เลื่อนบิตที่แปลงแล้วไปอยู่ในตำแหน่งที่ถูกต้องในข้อมูล 16-bit
    //    (Red อยู่ซ้ายสุด, Green อยู่กลาง, Blue อยู่ขวาสุด)
    // 3. นำค่ามารวมกันด้วย Bitwise OR (|)

    return ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
}
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

    ILI9341_Init();
    char rbar[10],gbar[10],bbar[10];
    // ฟังก์ชันวาดรูปภาพแบบกำหนดพิกัดและขนาด
    void ILI9341_Draw_Custom_Image(uint16_t X, uint16_t Y, uint16_t Width, uint16_t Height, const char* Image_Array) {
        // 1. ตรวจสอบไม่ให้ขนาดเกินขอบจอ
        if((X >= 320) || (Y >= 240)) return;
        if((X + Width - 1) >= 320) Width = 320 - X;
        if((Y + Height - 1) >= 240) Height = 240 - Y;

        // 2. ล็อกขอบเขตหน้าต่างบนหน้าจอที่จะพ่นสีลงไป
        ILI9341_Set_Address(X, Y, X + Width - 1, Y + Height - 1);

        // 3. พ่นข้อมูลสีจาก Array เข้าจอผ่าน SPI ทีละไบต์
        uint32_t total_bytes = (uint32_t)Width * Height * 2;
        for(uint32_t i = 0; i < total_bytes; i++) {
            ILI9341_Write_Data((uint8_t)Image_Array[i]);
        }
    }
    void clear_display(){
        	ILI9341_Fill_Screen(WHITE);
        }
    void display1(){
    					int Width = 121;
    					Read_AM2320();
    	    			sprintf(ctemper,"%.1f C",temperature);
    	    			sprintf(chumid,"%.1f %%RH",humidity);
    	    			ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
    	    			ILI9341_Draw_Text(ctemper, 20, 20, BLACK, 2, WHITE);
    	    			ILI9341_Draw_Text(chumid, 170, 20, BLACK, 2, WHITE);

    	    			ILI9341_Draw_Filled_Circle(130, 30, 25, RGB565_CONVERT(r*25,g*25,b*25));

    	    			ILI9341_Draw_Filled_Circle(30, 81, 20, RED);
    	    			ILI9341_Draw_Filled_Circle(30, 141, 20, GREEN);
    	    			ILI9341_Draw_Filled_Circle(30, 201, 20, BLUE);

    	    			ILI9341_Draw_Rectangle(60, 60, Width, 35, RGB565_CONVERT(255, 200, 200));
    	    			ILI9341_Draw_Rectangle(60, 120, Width, 35, RGB565_CONVERT(200, 255, 200));
    	    			ILI9341_Draw_Rectangle(60, 180, Width, 35, RGB565_CONVERT(200, 200, 255));

    	    			ILI9341_Draw_Rectangle(60, 60, (r%11)*12, 35, RED);
    	    			ILI9341_Draw_Rectangle(60, 120, (g%11)*12, 35, GREEN);
    	    			ILI9341_Draw_Rectangle(60, 180, (b%11)*12, 35, BLUE);

    	    			sprintf(rbar,"%d%%",(r%11)*10);
    	    			ILI9341_Draw_Text(rbar, 190, 70, BLACK, 3, WHITE);

    	    			sprintf(gbar,"%d%%",(g%11)*10);
    	    			ILI9341_Draw_Text(gbar, 190, 130, BLACK, 3, WHITE);

    	    			sprintf(bbar,"%d%%",(b%11)*10);
    	    			ILI9341_Draw_Text(bbar, 190, 190, BLACK, 3, WHITE);

    	    			if(TP_Touchpad_Pressed())
    	    			        {
    	    			            uint16_t pos[2];

    	    			            // 2. อ่านพิกัดที่กด
    	    			            if(TP_Read_Coordinates(pos) == TOUCHPAD_DATA_OK)
    	    			            {
    	    			                uint16_t x = pos[0];
    	    			                uint16_t y = 319-pos[1];
    	//    			                char coor[10];
    	//    			                sprintf(coor,"x = %d,y=%d",x,y);
    	//    			                ILI9341_Draw_Text(coor,y, x, BLACK,1,WHITE);
    	    			                // เอา X, Y ไปใช้แสดงหรือวาดของได้
    	    			                // เช่น วาดจุดตามนิ้ว
    	    			                if(y>10 && y< 50&& x>61 && x<101){
    	    			                	r++;
    	    			                	r%=11;
    	    			                	if(r==0)ILI9341_Fill_Screen(WHITE);

    	    			                }
    	    			                else if(y>10 && y< 50&& x>121 && x<161){
    	    			                	g++;
    										g%=11;
    										if(g==0)ILI9341_Fill_Screen(WHITE);
    	    			                    			                }
    	    			                else if(y>10 && y< 50&& x>181 && x<221){
    										b++;
    										b%=11;
    										if(b==0)ILI9341_Fill_Screen(WHITE);
    	    			                    			                }
    	    			                else if(y>105 && y< 155&& x>5 && x<55){
    	    			                	flagDisplay  = 0;
    	    			                	clear_display();
    	    			                	now = HAL_GetTick();

    	    			                }

    	    			            }
    	    			        }
    }

    void display2(){
    	ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
    	ILI9341_Draw_Text("Group No.16", 130, 20, RGB565_CONVERT(r*25,g*25,b*25), 2, WHITE);
    	ILI9341_Draw_Text("Tawan", 130, 40, RGB565_CONVERT(r*25,g*25,b*25), 2, WHITE);
    	ILI9341_Draw_Text("Khuntha", 130, 60, RGB565_CONVERT(r*25,g*25,b*25), 2, WHITE);
    	ILI9341_Draw_Text("67010338", 130, 80, RGB565_CONVERT(r*25,g*25,b*25), 2, WHITE);

//		ILI9341_Draw_Image((const char*)stdpic, SCREEN_HORIZONTAL_2);
		ILI9341_Draw_Custom_Image(0, 20, 120, 160, (const char*)stdpic);
		if(TP_Touchpad_Pressed())
								{
									uint16_t pos[2];

									// 2. อ่านพิกัดที่กด
									if(TP_Read_Coordinates(pos) == TOUCHPAD_DATA_OK)
									{
										uint16_t x = pos[0];
										uint16_t y = 319-pos[1];
		//    			                char coor[10];
		//    			                sprintf(coor,"x = %d,y=%d",x,y);
		//    			                ILI9341_Draw_Text(coor,y, x, BLACK,1,WHITE);
										// เอา X, Y ไปใช้แสดงหรือวาดของได้
										// เช่น วาดจุดตามนิ้ว
										if(y>10 && y< 130&& x>0 && x<160){
											flagDisplay  = 1;
											clear_display();

										}

									}
								}
    }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	ILI9341_Fill_Screen(WHITE);
    while (1)
      {
    	/* USER CODE END WHILE */

    	  /* USER CODE BEGIN 3 */
    	if(flagDisplay == 0 && HAL_GetTick()-now >=5000){
    		flagDisplay = 1;
        	clear_display();

    	}
    	if(flagDisplay==1){
    	display1();
    	}
    	else if(flagDisplay ==0){
    	display2();
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 216;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
  {
    Error_Handler();
  }
}

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
