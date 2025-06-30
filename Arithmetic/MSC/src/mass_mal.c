/**
  ******************************************************************************
  * @file    mass_mal.c
  * @author  MCD Application Team
  * @version V4.1.0
  * @date    26-May-2017
  * @brief   Medium Access Layer interface
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics International N.V. 
  * All rights reserved.</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */


/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "mass_mal.h"
#include "SPI_Flash.h"
#include "stdio.h"
#include "FlashLayout.h"
#include "led.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint32_t Mass_Memory_Offset[2];
uint32_t Mass_Memory_Size[2];
uint32_t Mass_Block_Size[2];
uint32_t Mass_Block_Count[2];

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
* Function Name  : MAL_Init
* Description    : Initializes the Media on the STM32
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint16_t MAL_Init(uint8_t lun)
{
    uint16_t stat = MAL_FAIL;

    switch (lun) {
        case 0: {
            Mass_Memory_Size[0]   = W25QXX_ReadCapacity() - SPI_FLASH_FILE_SYSTEM_ADDRESS;
            Mass_Memory_Offset[0] = SPI_FLASH_FILE_SYSTEM_ADDRESS;
            Mass_Block_Size[0]    = W25QXX_BLOCK_SIZE;
            Mass_Block_Count[0]   = Mass_Memory_Size[0] / Mass_Block_Size[0];

            if (Mass_Memory_Size[0] != 0x0000) {
                stat = MAL_OK;
            }
        } break;
    }

    return stat;
}
/*******************************************************************************
* Function Name  : MAL_Write
* Description    : Write sectors
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint16_t MAL_Write(uint8_t lun, uint32_t Memory_Offset, uint32_t *Writebuff, uint16_t Transfer_Length)
{
    uint16_t stat = MAL_FAIL;

    switch (lun) {
        case 0: {
            LED_On(ERR);
            Memory_Offset += Mass_Memory_Offset[0];
            while (Transfer_Length) {
                W25QXX_EraseSector(Memory_Offset);
                W25QXX_Write((uint8_t*) Writebuff,
                             Memory_Offset,
                             Mass_Block_Size[0]);
                Memory_Offset += Mass_Block_Size[0];
                Writebuff += Mass_Block_Size[0] / 4;
                Transfer_Length -= Mass_Block_Size[0];
            }
            LED_Off(ERR);
            stat = MAL_OK;
        } break;
    }

    return stat;
}

/*******************************************************************************
* Function Name  : MAL_Read
* Description    : Read sectors
* Input          : None
* * Output         : None
* Return         : Buffer pointer
*******************************************************************************/
uint16_t MAL_Read(uint8_t lun, uint32_t Memory_Offset, uint32_t *Readbuff, uint16_t Transfer_Length)
{
    uint16_t stat = MAL_FAIL;

    switch (lun) {
        case 0: {
            LED_On(ERR);
            Memory_Offset += Mass_Memory_Offset[0];
            W25QXX_Read((uint8_t*) Readbuff,
                        Memory_Offset,
                        Transfer_Length);
            LED_Off(ERR);
            stat = MAL_OK;
        } break;
    }
    return stat;
}

/*******************************************************************************
* Function Name  : MAL_GetStatus
* Description    : Get status
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint16_t MAL_GetStatus (uint8_t lun)
{
    uint16_t stat = MAL_FAIL;

    switch (lun) {
        case 0: {
            stat = MAL_OK;
        } break;
    }
    return stat;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

