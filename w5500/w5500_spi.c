#include "stm32l4xx_hal.h"
#include "wizchip_conf.h"
#include "stdio.h"


extern SPI_HandleTypeDef hspi1;


uint8_t SPIReadWrite(uint8_t data)
{


	while((hspi1.Instance->SR & SPI_FLAG_TXE) != SPI_FLAG_TXE);
	*(__IO uint8_t*)&hspi1.Instance->DR=data;

	while((hspi1.Instance->SR & SPI_FLAG_RXNE) !=SPI_FLAG_RXNE);

	return (*(__IO uint8_t*)&hspi1.Instance->DR);

}


void wizchip_select(void)
{
HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);	// Makes the pin low
}

void wizchip_deselect(void){

	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_1, GPIO_PIN_SET); // Makes the pin high

}

uint8_t wichip_read()
{

	uint8_t rb;
	rb = SPIReadWrite(0x00);
	return rb;
}

void wizchip_write(uint8_t wb)
{
	SPIReadWrite(wb);

}

void wizchip_readburst(uint8_t*  pBuf, uint_16_t len )
{
	for(uint_16_t i=0;i<len;i++)
	{
		*pBuf = SPIReadWrite(0x00);
		pBuf++;
	}
}

void wizchip_writeburst(uint8_t*  pBuf, uint_16_t len )
{
	for(uint_16_t i=0;i<len;i++)
	{
		SPIReadWrite(*pBuf);
		pBuf++;
	}
}

void W5500IOInit()
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	__HAL_RCC_GPIOA_CLK_ENABLE();
	GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}


void W5500Init()
{
	uint8_t tmp;
	uint8_t memsize[2][1] = {{16},{16}}; // We only need one socket so we can set 16kb for rx and 16 for tx. Total capacity is 32kb
	W5500IOInit();

	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_1, GPIO_PIN_SET); // Setting high by default (LUIS REMEMBER TO CHECK THIS SINCE THERE IS SOMETHING RELATED ON THE PACKET FORWARDER)
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_0,GPIO_PIN_RESET); // Reset the pin

	tmp = 0xFF;
	while(tmp--);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);

	reg_wizchip_cs_cbfunc(wizchip_select, wizchip_deselect); // Registering callback functions to select and deselect
	reg_wizchip_spi_cbfunc(wizchip_read, wizchip_write); // Registering SPI functions write and read
	reg_wizchip_spiburst_cbfunc(wizchip_readburst, wizchip_writeburst);

	if(ctlwizchip(CW_INIT_WIZCHIP,(void*) memsize) == -1) //Init the wizCHIP
	{
		printf('wizchip init failed \n');
		while(1);

	}

	printf('wizchip init success :) \n');







}
