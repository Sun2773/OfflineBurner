# OfflineBurner 离线编程器

[![Version](https://img.shields.io/badge/version-0.07-blue.svg)](https://github.com/Sun2773/OfflineBurner)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-STM32F103-orange.svg)](https://www.st.com/en/microcontrollers-microprocessors/stm32f103.html)
[![Hardware](https://img.shields.io/badge/hardware-开源-green.svg)](https://oshwhub.com/xiaoxunweixun/burner)

## 项目简介

OfflineBurner 是一款基于 STM32F103 的离线编程器，支持对各种 STM32 系列微控制器进行离线烧录。该编程器操作简单，免驱动，具有 U盘 拖拽式固件传输功能，可脱离 PC 独立完成目标芯片的程序烧录工作。

### 核心优势
- 🚀 **即插即用**: 免驱动，插入即可使用
- 📂 **拖拽式操作**: 如同普通 U盘，直接拖拽 .bin 文件
- 🔌 **离线工作**: 无需 PC 连接，独立完成烧录任务
- 💡 **智能提示**: 丰富的 LED 和声音状态反馈
- 🔓 **完全开源**: 软件和硬件设计完全开源，支持自由定制

## 主要特性

- 🔥 **离线烧录**: 支持脱离 PC 独立完成 STM32 芯片烧录
- 💾 **U盘模式**: 拖拽式固件文件传输，无需专用软件
- 🎯 **多芯片支持**: 支持 STM32F0/F1/F4 系列芯片
- ⚡ **SWD 接口**: 通过 SWD 协议进行目标芯片编程
- 🔧 **智能识别**: 自动检测目标板连接并触发烧录
- 🔊 **状态指示**: LED 和蜂鸣器丰富的状态提示
- 🛡️ **IAP 升级**: 支持通过 U盘 进行固件升级
- 🎮 **按键操作**: 支持手动触发烧录和 U盘 挂载

## 硬件架构

### 主控芯片
- **MCU**: STM32F103 系列
- **存储**: 外部 SPI Flash (16MB)
- **接口**: USB 2.0 Full Speed
- **调试**: SWD 接口

### 存储布局
```
SPI Flash 布局 (16MB):
┌─────────────────┐ 0x00000000
│   ConfigInfo    │ 配置信息区 (4KB)
├─────────────────┤ 0x00001000  
│  IAP Firmware   │ IAP固件区 (128KB)
├─────────────────┤ 0x00021000
│                 │
│   Free Space    │ 保留区域
│                 │
├─────────────────┤ 0x00100000
│                 │
│ Target Program  │ 目标程序存储区 (3MB)
│                 │
├─────────────────┤ 0x00400000
│                 │
│   File System   │ 文件系统区 (12MB)
│                 │
└─────────────────┘ 0x01000000
```

## 软件架构

### 核心模块

#### 烧录算法 (Arithmetic/algo)
- **flash_blob**: Flash 编程算法管理
- **STM32F0/F1/F4**: 各系列芯片专用算法

#### 目标通讯接口 (Arithmetic/SWD)
- **SWD**: SWD 协议实现和 Flash 操作

#### 上位机通讯接口 (Arithmetic/MSC)
- **MSC**: USB 大容量存储设备

#### 硬件驱动 (Board/)
- **SPI_Flash**: 外部 SPI Flash 驱动
- **Key**: 按键检测
- **LED**: 状态指示灯
- **Buzzer**: 蜂鸣器控制

#### 任务管理 (Task/)
- **Task_Burner**: 烧录任务管理
- **Task_Key**: 按键任务处理
- **Task_Led**: LED 状态控制
- **Task_USB**: USB 通信任务

## 支持的芯片

### STM32F0 系列
- STM32F030/031/042/048/051/058/070/071/072/078/091/098

### STM32F1 系列  
- STM32F100/101/102/103/105/107

### STM32F4 系列
- STM32F401/405/407/411/412/413/415/417/423/427/429/437/439/446/469/479

## 开发环境

### 必需工具
- **IDE**: IAR Embedded Workbench for ARM
- **调试器**: J-Link 或 ST-Link
- **Python**: Python 3.x (项目依赖 Python 脚本生成文件)

### 依赖库
- STM32F10x Standard Peripheral Library
- CMSIS Core
- CMSIS-DAP
- FatFS 文件系统
- cJSON 解析库
- USB FS Device Driver

## 快速开始

### 1. 获取项目
```bash
git clone https://github.com/Sun2773/OfflineBurner.git
cd OfflineBurner
```

### 2. 开发环境配置
1. 安装 IAR Embedded Workbench
2. 连接 J-Link 调试器
3. 打开 `Project.eww` 工作空间文件

### 3. 编译项目
**首次编译需先执行初始化脚本**：
```bash
./Script/init.bat  # 安装 Python 依赖
```

**使用 VS Code 任务**：
- 按 `Ctrl+Shift+P` 打开命令面板
- 运行 "Tasks: Run Task"
- 选择 "iar: Build Project"

**使用 IAR IDE**：
- 打开项目后选择对应配置 (Debug/Release/Bootloader)
- 点击编译按钮或按 F7

### 4. 烧录固件到编程器
**方式一：使用 IAR 直接下载**
- 需要先下载 Bootloader 配置
- 然后下载主程序

**方式二：使用脱机烧录器**
- 通过 J-Flash 或其他脱机烧录器
- 下载与 Bootloader 合并后的 .bin 文件

### 5. 设备固件升级
如需升级编程器固件：
1. 在编程器U盘中创建一个名为 `firmware` 的文件夹
2. 将新的固件文件放入 `firmware` 文件夹下
3. 重新为编程器上电，等待绿色指示灯500ms周期闪烁即升级完成
4. **注意**: 升级过程中红灯闪烁属正常现象，不要断电

### 6. 配置文件 (可选)
系统会在编程器 U盘 中自动生成配置文件 `config.json`：
```json
{
    "file": "",
    "version": "0.07",
    "autoBurn": 1,
    "chipErase": 0,
    "chipLock": 0,
    "autoRun": 1,
    "flashAddr": "0x08000000"
}
```
**说明**：
- 可修改此文件进行功能配置
- 删除文件后重新上电会生成默认配置

## 使用方法

## 使用方法

### 快速使用步骤
1. **连接电脑**：将编程器插入电脑 USB 口
2. **挂载 U盘**：长按按键直至蜂鸣器长鸣，U盘 挂载成功
3. **拖入固件**：将目标程序(.bin 文件)直接拖入 U盘 (可修改配置文件)
4. **断开重启**：重新为编程器上电，等待绿灯 500ms 周期闪烁(处理完成)
5. **连接目标**：通过 SWD 接口连接目标 STM32 单片机
6. **自动烧录**：连接目标板后自动触发烧录 (autoBurn=1)

### 手动操作
- **手动烧录**：未烧录状态下，单击功能键主动触发烧录
- **挂载 U盘**：长按功能键可重新挂载 U盘

### 重要注意事项
- ⚠️ **文件限制**: 只能放入一个 `*.bin` 文件
- ⚠️ **命名规范**: 文件名不可使用中文字符
- ⚠️ **升级安全**: 设备升级过程中严禁断电，否则设备变砖

### LED 和蜂鸣器状态指示

#### 蜂鸣器提示
- 🔊 **短鸣一声**: 开始烧录
- 🔊 **再次短鸣**: 烧录完成
- 🔊 **长鸣**: 编程失败 (同时红灯常亮)
- 🔊 **超短鸣**: 识别到目标板 (非自动烧录模式)

#### LED 状态指示
- 🟢 **绿灯闪烁**: 正在烧录
- 🟢 **绿灯500ms周期闪烁**: 设备空闲/处理完成
- 🔴 **红灯闪烁**: 正在处理文件
- 🔴 **红灯常亮**: 编程失败 (同时蜂鸣器长鸣)
- 🟢 **绿灯呼吸**: U盘模式

### 配置参数说明
| 参数 | 类型 | 说明 | 默认值 |
|------|------|------|--------|
| file | string | 当前烧录文件名 | "" |
| version | string | 固件版本 | "0.07" |
| autoBurn | number | 自动烧录使能 | 1 |
| chipErase | number | 全片擦除 | 0 |
| chipLock | number | 烧录后锁定 | 0 |
| autoRun | number | 烧录后自动运行 | 1 |
| flashAddr | string | 烧录起始地址 | "0x08000000" |

## 开发指南

### 添加新芯片支持

#### 准备算法文件
1. 在 MDK 软件包中寻找 Flash 编程算法文件 (`*.FLM` 格式)
2. 将算法文件复制到 `./Script/algo/` 路径下

#### 生成算法代码
3. 运行算法转换脚本：
   ```bash
   cd Script
   python algo.py
   ```
4. 选择要生成的算法文件，脚本会生成对应的 `*.c` 文件

#### 集成到项目
5. 在 `Arithmetic/algo/` 目录下创建芯片系列文件夹
6. 将生成的 `*.c` 文件复制到对应路径
7. 在 IAR 项目中添加新文件
8. 在 `flash_blob.c` 中注册新算法
9. 重新编译项目

### 调试和测试
- **实时日志**: 使用 RTT 查看调试信息
  ```bash
  JLinkRTTClient.exe
  ```
- **Flash 布局**: 查看 `FlashLayout.h` 了解存储分配
- **调试版本**: 修改 `Version.h` 中的 DEBUG 宏

## 项目结构
```
OfflineBurner/
├── Arithmetic/                 # 核心算法和通信协议
│   ├── algo/                   # Flash 编程算法
│   │   ├── STM32F0/           # STM32F0 系列算法
│   │   ├── STM32F1/           # STM32F1 系列算法
│   │   ├── STM32F4/           # STM32F4 系列算法
│   │   ├── flash_blob.c       # 算法管理
│   │   └── flash_blob.h
│   ├── MSC/                   # USB 大容量存储
│   ├── SWD/                   # SWD 调试接口
│   ├── BurnerConfig.c/.h      # 配置管理
│   ├── crc.c/.h              # CRC 校验
│   ├── heap.c/.h             # 内存管理
│   └── Tool.c/.h             # 工具函数
├── Board/                     # 硬件外设驱动
│   ├── SPI_Flash.c/.h        # SPI Flash 驱动
│   ├── Key.c/.h              # 按键检测
│   ├── led.c/.h              # LED 控制
│   └── buzzer.c/.h           # 蜂鸣器控制
├── Chip/                      # 芯片级驱动
├── Library/                   # 第三方库
│   ├── CMSIS/                # ARM CMSIS 库
│   ├── CMSIS-DAP/            # DAP 协议
│   ├── FATFS/                # 文件系统
│   ├── cJson/                # JSON 解析
│   └── STM32_USB-FS-Device_Driver/
├── Output/                    # 编译输出
│   ├── Debug/                # 调试版本
│   ├── Release/              # 发布版本
│   └── Bootloader/           # 引导程序
├── Project/                   # IAR 项目文件
├── Script/                    # 构建脚本
│   ├── algo/                 # 算法转换脚本
│   ├── init.bat              # 初始化脚本
│   ├── clean.bat             # 清理脚本
│   └── post-build.py         # 编译后处理
├── Task/                      # 任务管理模块
│   ├── Task_Burner.c/.h      # 烧录任务
│   ├── Task_Key.c/.h         # 按键任务
│   ├── Task_Led.c/.h         # LED 任务
│   └── Task_USB.c/.h         # USB 任务
├── User/                      # 用户应用代码
│   ├── main.c                # 主程序入口
│   ├── bootloader.c          # 引导程序
│   └── stm32f10x_it.c/.h     # 中断处理
├── FlashLayout.h              # Flash 布局定义
├── Version.h                  # 版本信息
├── Project.eww                # IAR 工作空间
├── LICENSE                    # 许可证
└── README.md                  # 项目说明
```

## 贡献指南

1. Fork 本项目
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 打开 Pull Request

## 常见问题

### Q: 烧录失败怎么办？
**检查清单**：
- ✅ 目标芯片连接是否正确
- ✅ SWD 接口线序是否正常
- ✅ 目标芯片型号是否在支持列表中
- ✅ 固件文件是否为有效的 .bin 格式
- ✅ 确保 U盘 中只有一个 .bin 文件
- ✅ 文件名是否包含中文字符

### Q: 如何正确使用 U盘 模式？
**操作流程**：
1. 长按功能键直至蜂鸣器长鸣 (挂载 U盘)
2. 将 .bin 文件拖入 U盘 根目录
3. 安全弹出 U盘 或直接拔出设备
4. 重新上电，等待绿灯 500ms 周期闪烁
5. 连接目标板，自动开始烧录

### Q: 设备升级失败变砖了怎么办？
**预防措施**：
- 升级过程中绝对不要断电
- 确保固件文件放在 `firmware` 文件夹中
- 使用官方提供的固件文件

**解决方法**：
- 通过 SWD 接口重新烧录 Bootloader
- 联系技术支持获取恢复固件

### Q: LED 和蜂鸣器状态含义？
**快速参考**：
| 状态 | LED | 蜂鸣器 | 含义 |
|------|-----|--------|------|
| 空闲 | 🟢 500ms 闪烁 | - | 设备就绪 |
| 处理文件 | 🔴 快闪 | - | 正在处理 |
| 烧录中 | 🟢 快闪 | 短鸣开始 | 正在烧录 |
| 烧录完成 | 🟢 500ms 闪烁 | 短鸣结束 | 烧录成功 |
| 烧录失败 | 🔴 常亮 | 长鸣 | 烧录失败 |
| 检测目标 | - | 超短鸣 | 识别到目标板 |

### Q: 如何添加新的芯片型号？
**参考步骤**：
- 查看"开发指南 > 添加新芯片支持"章节
- 使用 MDK 软件包中的 .FLM 算法文件
- 通过 Python 脚本转换为 C 代码

### Q: 配置文件格式错误？
**检查要点**：
- 确保 JSON 格式正确 (可使用在线 JSON 验证器)
- 所有字符串必须使用双引号
- 数值类型要匹配 (0/1 为数字，地址为字符串)
- 删除配置文件可恢复默认设置

## 版本历史

### v0.07 (当前版本) - 2025.07
- ✨ **新功能**: 添加自动烧录功能
- 🐛 **修复**: SWD 通信稳定性问题
- 📝 **改进**: 完善配置文件支持
- 🔧 **优化**: 任务调度机制

### v0.06 - 2024.12
- ✨ **新功能**: 支持 STM32F4 系列芯片
- 🔧 **优化**: 改进文件系统性能
- 📱 **新增**: USB MSC 接口支持

### v0.05 - 2024.09
- ✨ **里程碑**: 初始版本发布
- 🎯 **支持**: STM32F0/F1 系列基本烧录功能
- 🔧 **基础**: 核心烧录算法实现

## 许可证

本项目采用 [MIT 许可证](LICENSE) 开源发布。

### 许可证要点
- ✅ **自由使用**: 允许任何人免费使用、修改和分发
- ✅ **商业友好**: 可用于商业项目，无需支付费用
- ✅ **简单明了**: 条款简洁，易于理解和遵守
- ⚠️ **保留声明**: 使用时需保留原始版权和许可声明
- ⚠️ **免责条款**: 软件按"现状"提供，不提供任何保证

详细许可条款请查看 [LICENSE](LICENSE) 文件。

## 联系方式

- 📋 **项目主页**: [GitHub Repository](https://github.com/Sun2773/OfflineBurner)
- 🔧 **硬件开源**: [立创开源硬件平台](https://oshwhub.com/xiaoxunweixun/burner)
- 🐛 **问题反馈**: [Issues](https://github.com/Sun2773/OfflineBurner/issues)
- 📧 **技术支持**: 15863062773@163.com

---

**注意**: 本项目仅供学习和开发使用，请遵守相关法律法规和开源协议。