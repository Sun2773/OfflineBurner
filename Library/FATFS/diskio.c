/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
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
            if ((W25QXX_ReadID() != 0x0000) &&
                (W25QXX_ReadID() != 0xFFFF)) {
                /* 设备ID读取结果正确 */
                stat &= ~STA_NOINIT;
            }
            return stat;
        }
    }
    return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
    DSTATUS stat;

    switch (pdrv) {
        case DEV_SPI_FLASH: {
            W25QXX_Init();
            stat = disk_status(DEV_SPI_FLASH);
            return stat;
        }
    }
    return STA_NOINIT;
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
    DRESULT res;
    int     result;

    switch (pdrv) {
        case DEV_SPI_FLASH: {
            for (; count > 0; count--) {
                W25QXX_Read(buff,
                            sector * W25QXX_BLOCK_SIZE,
                            W25QXX_BLOCK_SIZE);
                sector++;
                buff += W25QXX_BLOCK_SIZE;
            }
            res = RES_OK;
            return res;
        }
    }

    return RES_PARERR;
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
    DRESULT res;

    switch (pdrv) {
        case DEV_SPI_FLASH: {
            for (; count > 0; count--) {
                W25QXX_EraseSector(sector * W25QXX_BLOCK_SIZE);
                W25QXX_Write((uint8_t*) buff,
                             sector * W25QXX_BLOCK_SIZE,
                             W25QXX_BLOCK_SIZE);
                sector++;
                buff += W25QXX_BLOCK_SIZE;
            }
            res = RES_OK;
            return res;
        }
    }

    return RES_PARERR;
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
    int     result;

    switch (pdrv) {
        case DEV_SPI_FLASH: {
            switch (cmd) {
                case GET_SECTOR_COUNT:
                    *(DWORD*) buff = W25QXX_ReadCapacity() / 4096;
                    break;
                    /* 扇区大小  */
                case GET_SECTOR_SIZE:
                    *(WORD*) buff = W25QXX_BLOCK_SIZE;
                    break;
                    /* 同时擦除扇区个数 */
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

