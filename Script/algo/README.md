# FLM Flash Algorithm Converter

这个工具可以解析ARM的FLM（Flash Load Module）文件，提取其中的Flash编程算法代码、FlashDevice配置和扇区信息，并生成C语言格式的flash blob文件。

## 功能特点

- ✅ 自动解析FLM文件中的PrgCode、PrgData、DevDscr段
- ✅ 提取Flash编程函数（Init、UnInit、EraseChip、EraseSector、ProgramPage）
- ✅ 解析FlashDevice结构体，获取芯片信息和扇区配置
- ✅ 生成符合flash_blob.h格式的C代码文件
- ✅ 支持单个文件处理或批量处理所有FLM文件
- ✅ 所有十六进制数严格使用大写格式
- ✅ 代码结构清晰，注释完整，易于理解和维护

## 系统要求

- Python 3.6+
- pyelftools库

## 安装依赖

```bash
pip install pyelftools
```

## 文件结构

```
.
├── main.py           # 主程序脚本
├── flash_blob.h      # C头文件定义
├── README.md         # 使用说明
├── *.FLM            # 输入的FLM文件
└── *.c              # 生成的C文件
```

## 使用方法

### 基本使用

1. **准备文件**：将FLM文件放在脚本所在目录

2. **运行脚本**：
   ```bash
   python main.py
   ```

3. **选择处理模式**：
   - 输入数字选择单个文件进行处理
   - 选择最后一个选项来批量处理所有FLM文件

### 示例操作

```bash
$ python main.py
Available FLM files:
  1.    STM32F10x_128.FLM
  2.    STM32F10x_512.FLM
  3.    STM32F4xx_128.FLM
  4.    Process all files
Select FLM file (number): 1

Processing FLM file: STM32F10x_128.FLM
Output file: STM32F10x_128.c
Found 5 functions:
  Init @ 0x00000001 in PrgCode (size: 50 bytes)
  UnInit @ 0x00000033 in PrgCode (size: 18 bytes)
  ...
✓ STM32F10x_128.FLM converted successfully!
```

## 输出文件格式

生成的C文件包含：

### 核心组件

- **`flash_code[]`** - Flash编程算法代码数组（包含特殊标记）
- **`sector_info[]`** - 扇区信息数组（大小和起始地址）
- **`program_target_t`** - 完整的程序目标结构体

### Flash Code数组特点

生成的`flash_code[]`数组具有以下结构：

```c
static const uint32_t flash_code[] = {
    // clang-format off
    0xE00ABE00,                                      // +0x0000 - 终止运行标记
    0x4603B510, 0x4C442000, 0x48446020, 0x48446060, // +0x0004 - 算法代码开始
    // ...更多算法代码...
    0x1CC01D00, 0x29001F09, 0x2000D1E5, 0x00000000  // +0x011C - 结束标记
    // clang-format on
};
```

**重要特性**：
- **开头标记**: `0xE00ABE00` - 用于终止运行
- **算法代码**: 实际的Flash编程算法机器码（小端序，大写十六进制）
- **结束标记**: `0x00000000` - 标记代码结束
- **地址偏移**: 所有注释显示正确的内存偏移量
- **对齐格式**: 注释严格对齐到第53列

### 地址计算说明

函数地址已自动调整为运行时地址：
- **基址**: `0x20000000` (SRAM起始)
- **偏移调整**: `+4字节` (因为添加了`0xE00ABE00`标记)
- **Thumb模式**: `+1` (所有函数地址末位置1)

例如：`Init`函数在文件中偏移为`0x00000001`，运行时地址为`0x20000005`

## 支持的芯片系列

工具已测试并支持以下ARM Cortex-M系列芯片：

### STM32F1系列
- **STM32F10x_16** - 16KB Flash (低密度)
- **STM32F10x_128** - 128KB Flash (中密度)  
- **STM32F10x_512** - 512KB Flash (高密度)
- **STM32F10x_1024** - 1024KB Flash (超高密度)
- **STM32F10x_CL** - 连接型系列
- **STM32F10x_OPT** - Option Bytes (选项字节)

### STM32F4系列
- **STM32F4xx_128** - 128KB Flash
- **STM32F4xx_256** - 256KB Flash
- **STM32F4xx_384** - 384KB Flash
- **STM32F4xx_512** - 512KB Flash
- **STM32F4xx_1024** - 1024KB Flash
- **STM32F4xx_1536** - 1536KB Flash
- **STM32F4xx_2048** - 2048KB Flash

### 其他支持
- 任何符合标准FLM格式的ARM Cortex-M芯片
- 自定义Flash算法（只要符合ARM标准FLM格式）

## 技术细节

### 内存布局标准

所有生成的代码都遵循标准的ARM Cortex-M SRAM布局：

```
0x20000000 ┌─────────────────┐
           │ Flash Algorithm │  <- 算法代码区
           │    Code         │
0x20000400 ├─────────────────┤
           │ Program Buffer  │  <- 数据缓冲区 (1024字节)
           │  (1024 bytes)   │
0x20000800 ├─────────────────┤
           │   Free Space    │  <- 可用空间
0x20000C00 ├─────────────────┤
           │  Static Data    │  <- 全局/静态变量区
0x20001000 ├─────────────────┤
           │     Stack       │  <- 栈空间 (向下增长)
           └─────────────────┘
```

### 关键地址定义

| 地址 | 用途 | 说明 |
|------|------|------|
| `0x20000000` | 算法代码起始 | Flash编程算法加载地址 |
| `0x20000400` | 编程缓冲区 | 1024字节数据缓冲区 |
| `0x20000C00` | 静态数据区 | 全局变量和静态数据 |
| `0x20001000` | 栈指针 | 栈空间起始地址 |

## 代码质量特性

### 格式标准
- ✅ **十六进制大写**: 所有十六进制数使用大写格式（如`0xABCD1234`）
- ✅ **注释对齐**: 行尾注释严格对齐到第53列
- ✅ **代码格式化**: 使用clang-format标记保护代码格式
- ✅ **结构一致**: 所有生成文件保持相同的代码结构

### 错误处理
- ✅ **文件验证**: 自动检查FLM文件完整性
- ✅ **段验证**: 确保必需的段（PrgCode、PrgData、DevDscr）存在
- ✅ **数据验证**: 验证FlashDevice结构体和扇区信息
- ✅ **错误报告**: 详细的错误信息和处理建议

## 支持的芯片

工具已测试支持：
- STM32F10x系列（16KB、128KB、512KB、1024KB等）
- STM32F4xx系列（128KB、256KB、384KB、512KB、1024KB、1536KB、2048KB等）
- 其他使用标准FLM格式的ARM芯片

## 完整示例输出

以下是`STM32F10x_128.c`的完整生成示例：

```c
#include "flash_blob.h"

/*
 * STM32F10x_128 SRAM布局 (基址0x20000000):
 *
 * 0x20000000 ┌─────────────────┐
 *            │ Flash Algorithm │  <- algo_start (算法代码)
 *            │    Code         │
 * 0x20000400 ├─────────────────┤
 *            │ Program Buffer  │  <- program_buffer (数据缓冲区)
 *            │  (1024 bytes)   │
 * 0x20000800 ├─────────────────┤
 *            │                 │
 *            │   Free Space    │
 *            │                 │
 * 0x20000C00 ├─────────────────┤
 *            │  Static Data    │  <- static_base (全局/静态变量)
 *            │     Area        │
 * 0x20001000 ├─────────────────┤
 *            │     Stack       │  <- stack_pointer (栈空间)
 *            │   (grows down)  │
 *            └─────────────────┘
 */

// Flash programming algorithm code
// Functions:
//   Init         @ +0x0005 (size: 50 bytes)
//   UnInit       @ +0x0037 (size: 18 bytes)
//   EraseChip    @ +0x0049 (size: 58 bytes)
//   EraseSector  @ +0x0083 (size: 62 bytes)
//   ProgramPage  @ +0x00C1 (size: 90 bytes)
static const uint32_t flash_code[] = {
    // clang-format off
    0xE00ABE00,                                      // +0x0000
    0x4603B510, 0x4C442000, 0x48446020, 0x48446060, // +0x0004
    0x46206060, 0xF01069C0, 0xD1080F04, 0x5055F245, // +0x0014
    // ...更多代码行...
    0x1CC01D00, 0x29001F09, 0x2000D1E5, 0x00000000  // +0x011C
    // clang-format on
};

// Flash sector information
sector_info_t sector_info[] = {
    {0x0400, 0x000000},
};

// Flash programming target configuration
const program_target_t _stm32f10x_128_flash_ = {
    0x20000005,   // Init
    0x20000037,   // UnInit
    0x20000049,   // EraseChip
    0x20000083,   // EraseSector
    0x200000C1,   // ProgramPage
    {
        0x20000001,   // BKPT : 断点地址 (算法起始+1，Thumb模式)
        0x20000C00,   // RSB  : 静态数据基址
        0x20001000,   // RSP  : 栈指针地址
    },
    0x20000400,           // 编程缓冲区地址
    0x20000000,           // 算法代码起始地址
    sizeof(flash_code),   // 算法代码大小
    flash_code,           // 算法代码数据指针
    0x00000400,           // 编程缓冲区大小

    sector_info,                                    // 扇区信息指针
    sizeof(sector_info) / sizeof(sector_info[0]),   // 扇区数量
};
```

## 常见问题与解决方案

### Q: 运行时提示"No FLM files found"
**A**: 确保FLM文件与`main.py`脚本在同一目录下，且文件扩展名为`.FLM`（大写）。

### Q: 生成的函数地址不正确
**A**: 工具自动处理地址计算，包括4字节偏移和Thumb模式调整。如有问题，请检查原始FLM文件的完整性。

### Q: 扇区信息解析失败
**A**: 检查FlashDevice结构体是否完整。工具需要完整的设备描述符段（DevDscr）来解析扇区信息。

### Q: 支持自定义芯片吗？
**A**: 支持！只要芯片使用标准的ARM FLM格式，工具就能正确解析和转换。

## 更新日志

### v2.0 (2025-06-27)
- ✅ 重构代码架构，提高可读性和维护性
- ✅ 优化函数结构，减少代码重复
- ✅ 完善错误处理和用户反馈
- ✅ 统一十六进制格式为大写
- ✅ 改进注释对齐和代码格式
- ✅ 更新文档，补充技术细节

### v1.0 (初始版本)
- ✅ 基本FLM文件解析功能
- ✅ 支持批量处理
- ✅ 生成标准C格式输出

## 许可证

MIT License - 自由使用和修改

## 贡献

欢迎提交Issue和Pull Request来改进这个工具！
