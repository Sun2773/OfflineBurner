/*
 * SPI_FLASH布局 :
 *
 * 0x00000000 ┌─────────────────┐
 *            │   ConfigInfo    │  <- 编程器配置信息
 * 0x00001000 ├─────────────────┤
 *            │  IAP Firmware   │  <- 用于对编程器进行固件升级
 * 0x00021000 ├─────────────────┤
 *            │                 │
 *            │   Free Space    │
 *            │                 │
 * 0x00100000 ├─────────────────┤
 *            │                 │
 *            │ Target Program  │  <- 存放用于烧录的目标程序
 *            │                 │
 * 0x00400000 ├─────────────────┤
 *            │                 │
 *            │   File System   │  <- 文件系统
 *            │                 │
 * 0x01000000 └─────────────────┘
 */

#define SPI_FLASH_CONFIG_ADDRESS      (0x00000000)   // 配置保存地址
#define SPI_FLASH_FIRMWARE_ADDRESS    (0x00001000)   // 固件保存地址
#define SPI_FLASH_PROGRAM_ADDRESS     (0x00100000)   // 程序保存地址
#define SPI_FLASH_FILE_SYSTEM_ADDRESS (0x00400000)   // 文件系统地址

/*
 * CHIP_FLASH布局 :
 *
 * 0x08000000 ┌─────────────────┐
 *            │    Bootloder    │  <- 引导程序
 * 0x08004000 ├─────────────────┤
 *            │                 │
 *            │    Firmware     │  <- 应用程序
 *            │                 │
 *            │                 │
 *            └─────────────────┘
 */

#define CHIP_BOOTLOADER_ADDRESS (0x08000000)                                       // 片内FLASH起始地址
#define CHIP_FIRMWARE_OFFSET    (0x00002000)                                       // 片内用户程序偏移地址
#define CHIP_FIRMWARE_ADDRESS   (CHIP_BOOTLOADER_ADDRESS + CHIP_FIRMWARE_OFFSET)   // 片内用户程序起始地址
