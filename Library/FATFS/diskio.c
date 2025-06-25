/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */
#include "SPI_Flash.h"

/* Definitions of physical drive number for each drive */
#define	DEV_SPI_FLASH	0	/* Example: Map SPI Flash to physical drive 0 */

uint32_t Fat_Memory_Offset[2];
uint32_t Fat_Memory_Size[2];
uint32_t Fat_Block_Size[2];
uint32_t Fat_Block_Count[2];

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
    DSTATUS stat = STA_NOINIT;

    switch (pdrv) {
        case DEV_SPI_FLASH: {
            uint16_t id = W25QXX_ReadID();
            if ((id != 0x0000) &&
                (id != 0xFFFF)) {
                /* �豸ID��ȡ�����ȷ */
                stat &= ~STA_NOINIT;
            }
        } break;
    }
    return stat;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
    DSTATUS stat = STA_NOINIT;

    switch (pdrv) {
        case DEV_SPI_FLASH: {
            /* �洢����ʼ�� */
            W25QXX_Init();

            Fat_Memory_Size[0]   = W25QXX_ReadCapacity() / 2;                  // Flash����
            Fat_Memory_Offset[0] = Fat_Memory_Size[0] / 2;                    // Flashƫ�Ƶ�ַ
            Fat_Block_Size[0]    = W25QXX_BLOCK_SIZE;                          // ���С
            Fat_Block_Count[0]   = Fat_Memory_Size[0] / Fat_Block_Size[0];   // ������

            stat = disk_status(pdrv);
        }
    }
    return stat;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
    DRESULT stat = RES_PARERR;

    switch (pdrv) {
        case DEV_SPI_FLASH: {
            sector += Fat_Memory_Offset[0] / Fat_Block_Size[0];
            /* �洢���� */
            W25QXX_Read(buff,
                        sector * Fat_Block_Size[0],
                        count * Fat_Block_Size[0]);

            stat = RES_OK;
        } break;
    }

    return stat;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
    DRESULT stat = RES_PARERR;

    switch (pdrv) {
        case DEV_SPI_FLASH: {
            sector += Fat_Memory_Offset[0] / Fat_Block_Size[0]; // ���ƫ�Ƶ�ַ
            while (count--) {
                W25QXX_EraseSector(sector * Fat_Block_Size[0]);
                W25QXX_Write((uint8_t*) buff,
                             sector * Fat_Block_Size[0],
                             Fat_Block_Size[0]);
                sector++;
                buff += Fat_Block_Size[0];
            }
            stat = RES_OK;
        } break;
    }

    return stat;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
    DRESULT res;

    switch (pdrv) {
        case DEV_SPI_FLASH: {
            switch (cmd) {
                case GET_SECTOR_COUNT:
                    *(DWORD*) buff = Fat_Block_Count[0]; // ���ؿ�����
                    break;
                    /* ������С  */
                case GET_SECTOR_SIZE:
                    *(WORD*) buff = Fat_Block_Size[0];
                    break;
                    /* ͬʱ������������ */
                case GET_BLOCK_SIZE:
                    *(DWORD*) buff = 1;
                    break;
            }
            res = RES_OK;
            return res;
        }
    }

    return RES_PARERR;
}

