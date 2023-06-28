/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for recieve_from_br */
osThreadId_t recieve_from_brHandle;
const osThreadAttr_t recieve_from_br_attributes = {
  .name = "recieve_from_br",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for send_to_bridge */
osThreadId_t send_to_bridgeHandle;
const osThreadAttr_t send_to_bridge_attributes = {
  .name = "send_to_bridge",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void recieveFromBridge(void *argument);
void sendToBridge(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
	 int32_t socket_result;
	 uint8_t socket_number = 0; 		// Set up socket number to use
	 uint16_t port = 1234; 			// PORT to connect to
	 uint8_t bridge_ip[4] = {0,0,0,0}; // IP to connect to
	 socket_str_t* bridge_connection;
	 brigde_connection = socket_factory(socket_number, bridge_ip, port);

	 printf('**********************************************\r\n');
	 printf('Initializing socket connection with the bridge\r\n');
	 printf('**********************************************\r\n');

	 socket_result = ETHERNET_SOCKET_Init(brigde_connection);
	 switch(ETHERNET_SOCKET_Init(brigde_connection))
	  {
	  case SOCKERR_SOCKNUM:
		  printf('xxxxxxxxxxxxxxxxxxxxx\r\n');
		  printf('Invalid socket number\r\n');
		  printf('xxxxxxxxxxxxxxxxxxxxx\r\n');
		  exit -1;
	  case SOCKERR_SOCKMODE:
		  printf('xxxxxxxxxxxxxxxxxx\r\n');
		  printf('Mode not supported\r\n');
		  printf('xxxxxxxxxxxxxxxxxx\r\n');
		  exit -1;
	  case SOCKERR_SOCKFLAG:
		  printf('xxxxxxxxxxxxxxxxxxxxx\r\n');
		  printf('Socket flag not valid\r\n');
		  printf('xxxxxxxxxxxxxxxxxxxxx\r\n');
		  exit -1;
	  }

		printf('**************\r\n');
		printf('Socket created\r\n');
		printf('**************\r\n');


	  for(uint_16_t i=0;i<10;i++)
	  	{
		  printf('*********************\r\n');
		  printf('Connecting to bridge \r\n');
		  printf('Attempting to connect %d \r\n', i);
		  printf('*********************\r\n');
		  socket_result = establish_connection_with_bridge(brigde_connection);
		  if(socket_result){
			  printf('***********\r\n');
			  printf('Connected! \r\n');
			  printf('***********\r\n');
			  break;
		  }
	  	}
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of recieve_from_br */
  recieve_from_brHandle = osThreadNew(recieveFromBridge, NULL, &recieve_from_br_attributes);

  /* creation of send_to_bridge */
  send_to_bridgeHandle = osThreadNew(sendToBridge, NULL, &send_to_bridge_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_recieveFromBridge */
/**
  * @brief  Function implementing the recieve_from_br thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_recieveFromBridge */
void recieveFromBridge(void *argument)
{
  /* USER CODE BEGIN recieveFromBridge */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END recieveFromBridge */
}

/* USER CODE BEGIN Header_sendToBridge */
/**
* @brief Function implementing the send_to_bridge thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_sendToBridge */
void sendToBridge(void *argument)
{
  /* USER CODE BEGIN sendToBridge */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END sendToBridge */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

