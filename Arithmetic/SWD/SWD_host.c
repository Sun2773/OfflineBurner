/**
 * @file    SWDd_host.c
 * @brief   Host driver for accessing the DAP
 */

#include "swd_host.h"
#include "DAP.h"
#include "DAP_config.h"
#include "debug_cm.h"

#define NVIC_Addr (0xe000e000)
#define DBG_Addr  (0xe000edf0)

// AP CSW register, base value
#define CSW_VALUE (CSW_RESERVED | CSW_MSTRDBG | CSW_HPROT | CSW_DBGSTAT | CSW_SADDRINC)

// SWD register access
#define SWD_REG_AP     (1)
#define SWD_REG_DP     (0)
#define SWD_REG_R      (1 << 1)
#define SWD_REG_W      (0 << 1)
#define SWD_REG_ADR(a) (a & 0x0c)

#define DCRDR  0xE000EDF8
#define DCRSR  0xE000EDF4
#define DHCSR  0xE000EDF0
#define REGWnR (1 << 16)

#define MAX_SWD_RETRY 10
#define MAX_TIMEOUT   1000000   // Timeout for syscalls on target

// #define SCB_AIRCR_PRIGROUP_Pos              8
// #define SCB_AIRCR_PRIGROUP_Msk             (7UL << SCB_AIRCR_PRIGROUP_Pos)

static DAP_STATE dap_state;

/**
 * @brief  延时函数
 * @note
 * @param  ms: 延时的毫秒数
 * @retval None
 */
void delaymS(uint32_t ms) {
    uint32_t cnt = SystemCoreClock / 4 / 1000 * ms;

    for (uint32_t i = 0; i < cnt; i++) {
        __NOP();
    }
}

/**
 * @brief  将32位整数转换为字节数组
 * @note   按小端格式转换
 * @param  res: 输出的字节数组
 * @param  data: 要转换的32位数据
 * @param  len: 转换的字节长度
 * @retval None
 */
static void int2array(uint8_t* res, uint32_t data, uint8_t len) {
    uint8_t i = 0;

    for (i = 0; i < len; i++) {
        res[i] = (data >> 8 * i) & 0xFF;
    }
}

/**
 * @brief  SWD传输重试函数
 * @note   当传输返回等待状态时会重试
 * @param  req: SWD请求
 * @param  data: 传输数据指针
 * @retval 传输应答状态
 */
static uint8_t swd_transfer_retry(uint32_t req, uint32_t* data) {
    uint8_t i, ack;

    for (i = 0; i < MAX_SWD_RETRY; i++) {
        ack = SWD_Transfer(req, data);

        if (ack != DAP_TRANSFER_WAIT) {
            return ack;
        }
    }

    return ack;
}

/**
 * @brief  初始化SWD接口
 * @note   配置DAP和SWD端口
 * @param  None
 * @retval 0: 成功, -1: 失败
 */
int8_t swd_init(void) {
    DAP_Setup();
    PORT_SWD_SETUP();

    return 0;
}

/**
 * @brief  关闭SWD接口
 * @note   关闭端口电源
 * @param  None
 * @retval 0: 成功, -1: 失败
 */
int8_t swd_off(void) {
    PORT_OFF();

    return 0;
}

/**
 * @brief  读取调试端口寄存器
 * @note   读取DP寄存器的值
 * @param  adr: 寄存器地址
 * @param  val: 读取到的值
 * @retval 0: 成功, -1: 失败
 */
int8_t swd_read_dp(uint8_t adr, uint32_t* val) {
    uint32_t tmp_in;
    uint8_t  tmp_out[4];
    uint8_t  ack;
    uint32_t tmp;

    tmp_in = SWD_REG_DP | SWD_REG_R | SWD_REG_ADR(adr);
    ack    = swd_transfer_retry(tmp_in, (uint32_t*) tmp_out);

    *val = 0;
    tmp  = tmp_out[3];
    *val |= (tmp << 24);
    tmp = tmp_out[2];
    *val |= (tmp << 16);
    tmp = tmp_out[1];
    *val |= (tmp << 8);
    tmp = tmp_out[0];
    *val |= (tmp << 0);

    return (ack == 0x01) ? 0 : -1;
}

/**
 * @brief  写入调试端口寄存器
 * @note   写入DP寄存器的值
 * @param  adr: 寄存器地址
 * @param  val: 要写入的值
 * @retval 0: 成功, -1: 失败
 */
int8_t swd_write_dp(uint8_t adr, uint32_t val) {
    uint32_t req;
    uint8_t  data[4];
    uint8_t  ack;

    switch (adr) {
        case DP_SELECT:
            if (dap_state.select == val) {
                return 0;
            }

            dap_state.select = val;
            break;

        default:
            break;
    }

    req = SWD_REG_DP | SWD_REG_W | SWD_REG_ADR(adr);
    int2array(data, val, 4);
    ack = swd_transfer_retry(req, (uint32_t*) data);

    return (ack == 0x01) ? 0 : -1;
}

/**
 * @brief  读取访问端口寄存器
 * @note   读取AP寄存器的值
 * @param  adr: 寄存器地址
 * @param  val: 读取到的值
 * @retval 0: 成功, -1: 失败
 */
int8_t swd_read_ap(uint32_t adr, uint32_t* val) {
    uint8_t  tmp_in, ack;
    uint8_t  tmp_out[4];
    uint32_t tmp;
    uint32_t apsel    = adr & 0xff000000;
    uint32_t bank_sel = adr & APBANKSEL;

    if (swd_write_dp(DP_SELECT, apsel | bank_sel) != 0) {
        return -1;
    }

    tmp_in = SWD_REG_AP | SWD_REG_R | SWD_REG_ADR(adr);
    // first dummy read
    swd_transfer_retry(tmp_in, (uint32_t*) tmp_out);
    ack = swd_transfer_retry(tmp_in, (uint32_t*) tmp_out);

    *val = 0;
    tmp  = tmp_out[3];
    *val |= (tmp << 24);
    tmp = tmp_out[2];
    *val |= (tmp << 16);
    tmp = tmp_out[1];
    *val |= (tmp << 8);
    tmp = tmp_out[0];
    *val |= (tmp << 0);

    return (ack == 0x01) ? 0 : -1;
}

/**
 * @brief  写入访问端口寄存器
 * @note   写入AP寄存器的值
 * @param  adr: 寄存器地址
 * @param  val: 要写入的值
 * @retval 0: 成功, -1: 失败
 */
int8_t swd_write_ap(uint32_t adr, uint32_t val) {
    uint8_t  data[4];
    uint8_t  req, ack;
    uint32_t apsel    = adr & 0xff000000;
    uint32_t bank_sel = adr & APBANKSEL;

    if (swd_write_dp(DP_SELECT, apsel | bank_sel) != 0) {
        return -1;
    }

    switch (adr) {
        case AP_CSW:
            if (dap_state.csw == val) {
                return 0;
            }

            dap_state.csw = val;
            break;

        default:
            break;
    }

    req = SWD_REG_AP | SWD_REG_W | SWD_REG_ADR(adr);
    int2array(data, val, 4);

    if (swd_transfer_retry(req, (uint32_t*) data) != 0x01) {
        return -1;
    }

    req = SWD_REG_DP | SWD_REG_R | SWD_REG_ADR(DP_RDBUFF);
    ack = swd_transfer_retry(req, NULL);

    return (ack == 0x01) ? 0 : -1;
}

/**
 * @brief  写入32位字对齐数据块到目标内存
 * @note   使用地址自增模式，大小以字节为单位
 * @param  address: 目标地址
 * @param  data: 数据指针
 * @param  size: 数据大小（字节）
 * @retval 0: 成功, -1: 失败
 */
static int8_t swd_write_block(uint32_t address, uint8_t* data, uint32_t size) {
    uint8_t  tmp_in[4], req;
    uint32_t size_in_words;
    uint32_t i, ack;

    if (size == 0) {
        return -1;
    }

    size_in_words = size / 4;

    // CSW register
    if (swd_write_ap(AP_CSW, CSW_VALUE | CSW_SIZE32) != 0) {
        return -1;
    }

    // TAR write
    req = SWD_REG_AP | SWD_REG_W | (1 << 2);
    int2array(tmp_in, address, 4);

    if (swd_transfer_retry(req, (uint32_t*) tmp_in) != 0x01) {
        return -1;
    }

    // DRW write
    req = SWD_REG_AP | SWD_REG_W | (3 << 2);

    for (i = 0; i < size_in_words; i++) {
        if (swd_transfer_retry(req, (uint32_t*) data) != 0x01) {
            return -1;
        }

        data += 4;
    }

    // dummy read
    req = SWD_REG_DP | SWD_REG_R | SWD_REG_ADR(DP_RDBUFF);
    ack = swd_transfer_retry(req, NULL);
    return (ack == 0x01) ? 0 : -1;
}

/**
 * @brief  从目标内存读取32位字对齐数据块
 * @note   使用地址自增模式，大小以字节为单位
 * @param  address: 目标地址
 * @param  data: 数据缓冲区指针
 * @param  size: 数据大小（字节）
 * @retval 0: 成功, -1: 失败
 */
static int8_t swd_read_block(uint32_t address, uint8_t* data, uint32_t size) {
    uint8_t  tmp_in[4], req, ack;
    uint32_t size_in_words;
    uint32_t i;

    if (size == 0) {
        return -1;
    }

    size_in_words = size / 4;

    if (swd_write_ap(AP_CSW, CSW_VALUE | CSW_SIZE32) != 0) {
        return -1;
    }

    // TAR write
    req = SWD_REG_AP | SWD_REG_W | AP_TAR;
    int2array(tmp_in, address, 4);

    if (swd_transfer_retry(req, (uint32_t*) tmp_in) != DAP_TRANSFER_OK) {
        return -1;
    }

    // read data
    req = SWD_REG_AP | SWD_REG_R | AP_DRW;

    // initiate first read, data comes back in next read
    if (swd_transfer_retry(req, NULL) != 0x01) {
        return -1;
    }

    for (i = 0; i < (size_in_words - 1); i++) {
        if (swd_transfer_retry(req, (uint32_t*) data) != DAP_TRANSFER_OK) {
            return -1;
        }

        data += 4;
    }

    // read last word
    req = SWD_REG_DP | SWD_REG_R | SWD_REG_ADR(DP_RDBUFF);
    ack = swd_transfer_retry(req, (uint32_t*) data);
    return (ack == 0x01) ? 0 : -1;
}

/**
 * @brief  读取目标内存数据
 * @note   读取单个32位字数据
 * @param  addr: 目标地址
 * @param  val: 读取到的值
 * @retval 0: 成功, -1: 失败
 */
static int8_t swd_read_data(uint32_t addr, uint32_t* val) {
    uint8_t  tmp_in[4];
    uint8_t  tmp_out[4];
    uint8_t  req, ack;
    uint32_t tmp;
    // put addr in TAR register
    int2array(tmp_in, addr, 4);
    req = SWD_REG_AP | SWD_REG_W | (1 << 2);

    if (swd_transfer_retry(req, (uint32_t*) tmp_in) != 0x01) {
        return -1;
    }

    // read data
    req = SWD_REG_AP | SWD_REG_R | (3 << 2);

    if (swd_transfer_retry(req, (uint32_t*) tmp_out) != 0x01) {
        return -1;
    }

    // dummy read
    req  = SWD_REG_DP | SWD_REG_R | SWD_REG_ADR(DP_RDBUFF);
    ack  = swd_transfer_retry(req, (uint32_t*) tmp_out);
    *val = 0;
    tmp  = tmp_out[3];
    *val |= (tmp << 24);
    tmp = tmp_out[2];
    *val |= (tmp << 16);
    tmp = tmp_out[1];
    *val |= (tmp << 8);
    tmp = tmp_out[0];
    *val |= (tmp << 0);
    return (ack == 0x01) ? 0 : -1;
}

/**
 * @brief  写入目标内存数据
 * @note   写入单个32位字数据
 * @param  address: 目标地址
 * @param  data: 要写入的数据
 * @retval 0: 成功, -1: 失败
 */
static int8_t swd_write_data(uint32_t address, uint32_t data) {
    uint8_t tmp_in[4];
    uint8_t req, ack;
    // put addr in TAR register
    int2array(tmp_in, address, 4);
    req = SWD_REG_AP | SWD_REG_W | (1 << 2);

    if (swd_transfer_retry(req, (uint32_t*) tmp_in) != 0x01) {
        return -1;
    }

    // write data
    int2array(tmp_in, data, 4);
    req = SWD_REG_AP | SWD_REG_W | (3 << 2);

    if (swd_transfer_retry(req, (uint32_t*) tmp_in) != 0x01) {
        return -1;
    }

    // dummy read
    req = SWD_REG_DP | SWD_REG_R | SWD_REG_ADR(DP_RDBUFF);
    ack = swd_transfer_retry(req, NULL);
    return (ack == 0x01) ? 0 : -1;
}

/**
 * @brief  从目标内存读取32位字
 * @note   读取单个32位字数据
 * @param  addr: 目标地址
 * @param  val: 读取到的值
 * @retval 0: 成功, -1: 失败
 */
static int8_t swd_read_word(uint32_t addr, uint32_t* val) {
    if (swd_write_ap(AP_CSW, CSW_VALUE | CSW_SIZE32) != 0) {
        return -1;
    }

    if (swd_read_data(addr, val) != 0) {
        return -1;
    }

    return 0;
}

/**
 * @brief  向目标内存写入32位字
 * @note   写入单个32位字数据
 * @param  addr: 目标地址
 * @param  val: 要写入的值
 * @retval 0: 成功, -1: 失败
 */
static int8_t swd_write_word(uint32_t addr, uint32_t val) {
    if (swd_write_ap(AP_CSW, CSW_VALUE | CSW_SIZE32) != 0) {
        return -1;
    }

    if (swd_write_data(addr, val) != 0) {
        return -1;
    }

    return 0;
}

/**
 * @brief  从目标内存读取8位字节
 * @note   读取单个8位字节数据
 * @param  addr: 目标地址
 * @param  val: 读取到的值
 * @retval 0: 成功, -1: 失败
 */
static int8_t swd_read_byte(uint32_t addr, uint8_t* val) {
    uint32_t tmp;

    if (swd_write_ap(AP_CSW, CSW_VALUE | CSW_SIZE8) != 0) {
        return -1;
    }

    if (swd_read_data(addr, &tmp) != 0) {
        return -1;
    }

    *val = (uint8_t) (tmp >> ((addr & 0x03) << 3));
    return 0;
}

/**
 * @brief  向目标内存写入8位字节
 * @note   写入单个8位字节数据
 * @param  addr: 目标地址
 * @param  val: 要写入的值
 * @retval 0: 成功, -1: 失败
 */
static int8_t swd_write_byte(uint32_t addr, uint8_t val) {
    uint32_t tmp;

    if (swd_write_ap(AP_CSW, CSW_VALUE | CSW_SIZE8) != 0) {
        return -1;
    }

    tmp = val << ((addr & 0x03) << 3);

    if (swd_write_data(addr, tmp) != 0) {
        return -1;
    }

    return 0;
}

/**
 * @brief  从目标内存读取非对齐数据
 * @note   可读取任意地址和大小的数据，大小以字节为单位
 * @param  address: 目标地址
 * @param  data: 数据缓冲区指针
 * @param  size: 数据大小（字节）
 * @retval 0: 成功, -1: 失败
 */
int8_t swd_read_memory(uint32_t address, uint8_t* data, uint32_t size) {
    uint32_t n;

    // Read bytes until word aligned
    while ((size > 0) && (address & 0x3)) {
        if (swd_read_byte(address, data) != 0) {
            return -1;
        }

        address++;
        data++;
        size--;
    }

    // Read word aligned blocks
    while (size > 3) {
        n = size & 0xFFFFFFFC;   // Only count complete words remaining

        if (swd_read_block(address, data, n) != 0) {
            return -1;
        }

        address += n;
        data += n;
        size -= n;
    }

    // Read remaining bytes
    while (size > 0) {
        if (swd_read_byte(address, data) != 0) {
            return -1;
        }

        address++;
        data++;
        size--;
    }

    return 0;
}

/**
 * @brief  向目标内存写入非对齐数据
 * @note   可写入任意地址和大小的数据，大小以字节为单位
 * @param  address: 目标地址
 * @param  data: 数据指针
 * @param  size: 数据大小（字节）
 * @retval 0: 成功, -1: 失败
 */
int8_t swd_write_memory(uint32_t address, uint8_t* data, uint32_t size) {
    uint32_t n = 0;

    // Write bytes until word aligned
    while ((size > 0) && (address & 0x3)) {
        if (swd_write_byte(address, *data) != 0) {
            return -1;
        }

        address++;
        data++;
        size--;
    }

    // Write word aligned blocks
    while (size > 3) {
        n = size & 0xFFFFFFFC;   // Only count complete words remaining

        if (swd_write_block(address, data, n) != 0) {
            return -1;
        }

        address += n;
        data += n;
        size -= n;
    }

    // Write remaining bytes
    while (size > 0) {
        if (swd_write_byte(address, *data) != 0) {
            return -1;
        }

        address++;
        data++;
        size--;
    }

    return 0;
}

/**
 * @brief  写入调试状态到目标核心
 * @note   设置核心寄存器状态用于系统调用执行
 * @param  state: 调试状态结构体指针
 * @retval 0: 成功, -1: 失败
 */
int8_t swd_write_debug_state(DEBUG_STATE* state) {
    uint32_t i, status;

    if (swd_write_dp(DP_SELECT, 0) != 0) {
        return -1;
    }

    // R0, R1, R2, R3
    for (i = 0; i < 4; i++) {
        if (swd_write_core_register(i, state->r[i]) != 0) {
            return -1;
        }
    }

    // R9
    if (swd_write_core_register(9, state->r[9]) != 0) {
        return -1;
    }

    // R13, R14, R15
    for (i = 13; i < 16; i++) {
        if (swd_write_core_register(i, state->r[i]) != 0) {
            return -1;
        }
    }

    // xPSR
    if (swd_write_core_register(16, state->xpsr) != 0) {
        return -1;
    }

    if (swd_write_word(DBG_HCSR, DBGKEY | C_DEBUGEN) != 0) {
        return -1;
    }

    // check status
    if (swd_read_dp(DP_CTRL_STAT, &status) != 0) {
        return -1;
    }

    if (status & (STICKYERR | WDATAERR)) {
        return -1;
    }

    return 0;
}

/**
 * @brief  读取核心寄存器
 * @note   读取ARM核心寄存器的值
 * @param  n: 寄存器编号
 * @param  val: 读取到的值
 * @retval 0: 成功, -1: 失败
 */
int8_t swd_read_core_register(uint32_t n, uint32_t* val) {
    int i = 0, timeout = 100;

    if (swd_write_word(DCRSR, n) != 0) {
        return -1;
    }

    // wait for S_REGRDY
    for (i = 0; i < timeout; i++) {
        if (swd_read_word(DHCSR, val) != 0) {
            return -1;
        }

        if (*val & S_REGRDY) {
            break;
        }
    }

    if (i == timeout) {
        return -1;
    }

    if (swd_read_word(DCRDR, val) != 0) {
        return -1;
    }

    return 0;
}

/**
 * @brief  写入核心寄存器
 * @note   写入ARM核心寄存器的值
 * @param  n: 寄存器编号
 * @param  val: 要写入的值
 * @retval 0: 成功, -1: 失败
 */
int8_t swd_write_core_register(uint32_t n, uint32_t val) {
    int i = 0, timeout = 100;

    if (swd_write_word(DCRDR, val) != 0) {
        return -1;
    }

    if (swd_write_word(DCRSR, n | REGWnR) != 0) {
        return -1;
    }

    // wait for S_REGRDY
    for (i = 0; i < timeout; i++) {
        if (swd_read_word(DHCSR, &val) != 0) {
            return -1;
        }

        if (val & S_REGRDY) {
            return 0;
        }
    }

    return -1;
}

/**
 * @brief  等待目标停止
 * @note   等待目标核心进入停止状态
 * @param  None
 * @retval 0: 成功, -1: 失败
 */
int8_t swd_wait_until_halted(void) {
    // Wait for target to stop
    uint32_t val, i, timeout = MAX_TIMEOUT;

    for (i = 0; i < timeout; i++) {
        if (swd_read_word(DBG_HCSR, &val) != 0) {
            return -1;
        }

        if (val & S_HALT) {
            return 0;
        }
    }

    return -1;
}

/**
 * @brief  执行Flash系统调用
 * @note   在目标核心上执行Flash算法函数并等待结果
 * @param  sysCallParam: 系统调用参数
 * @param  entry: 入口点地址
 * @param  arg1: 参数1
 * @param  arg2: 参数2
 * @param  arg3: 参数3
 * @param  arg4: 参数4
 * @retval 系统调用返回值
 */
int32_t swd_flash_syscall_exec(const program_syscall_t* sysCallParam, uint32_t entry, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4) {
    DEBUG_STATE state = {{0}, 0};
    // Call flash algorithm function on target and wait for result.
    state.r[0]  = arg1;                          // R0: Argument 1
    state.r[1]  = arg2;                          // R1: Argument 2
    state.r[2]  = arg3;                          // R2: Argument 3
    state.r[3]  = arg4;                          // R3: Argument 4
    state.r[9]  = sysCallParam->static_base;     // SB: Static Base
    state.r[13] = sysCallParam->stack_pointer;   // SP: Stack Pointer
    state.r[14] = sysCallParam->breakpoint;      // LR: Exit Point
    state.r[15] = entry;                         // PC: Entry Point
    state.xpsr  = 0x01000000;                    // xPSR: T = 1, ISR = 0

    if (swd_write_debug_state(&state) != 0) {
        return -1;
    }

    if (swd_wait_until_halted() != 0) {
        return -1;
    }

    if (swd_read_core_register(0, &state.r[0]) != 0) {
        return -1;
    }

    return state.r[0];
}

/**
 * @brief  SWD复位序列
 * @note   发送SWD复位序列
 * @param  None
 * @retval 0: 成功, -1: 失败
 */
static int8_t swd_reset(void) {
    uint8_t tmp_in[8];
    uint8_t i = 0;

    for (i = 0; i < 8; i++) {
        tmp_in[i] = 0xff;
    }

    SWJ_Sequence(51, tmp_in);
    return 0;
}

/**
 * @brief  SWD切换序列
 * @note   发送JTAG到SWD切换序列
 * @param  val: 切换序列值
 * @retval 0: 成功, -1: 失败
 */
static int8_t swd_switch(uint16_t val) {
    uint8_t tmp_in[2];
    tmp_in[0] = val & 0xff;
    tmp_in[1] = (val >> 8) & 0xff;
    SWJ_Sequence(16, tmp_in);
    return 0;
}

/**
 * @brief  读取SWD ID代码
 * @note   读取目标设备的ID代码
 * @param  id: 读取到的ID代码
 * @retval 0: 成功, -1: 失败
 */
int8_t swd_read_idcode(uint32_t* id) {
    uint8_t tmp_in[1];
    uint8_t tmp_out[4];
    tmp_in[0] = 0x00;
    SWJ_Sequence(8, tmp_in);

    if (swd_read_dp(0, (uint32_t*) tmp_out) != 0) {
        return -1;
    }

    *id = (tmp_out[3] << 24) | (tmp_out[2] << 16) | (tmp_out[1] << 8) | tmp_out[0];
    return 0;
}

/**
 * @brief  JTAG到SWD转换
 * @note   执行从JTAG到SWD的转换序列
 * @param  None
 * @retval 0: 成功, -1: 失败
 */
static int8_t JTAG2SWD() {
    uint32_t tmp = 0;

    if (swd_reset() != 0) {
        return -1;
    }

    if (swd_switch(0xE79E) != 0) {
        return -1;
    }

    if (swd_reset() != 0) {
        return -1;
    }

    if (swd_read_idcode(&tmp) != 0) {
        return -1;
    }

    return 0;
}

/**
 * @brief  初始化SWD调试接口
 * @note   初始化调试端口并建立调试连接
 * @param  None
 * @retval 0: 成功, -1: 失败
 */
int8_t swd_init_debug(void) {
    uint32_t tmp     = 0;
    int      i       = 0;
    int      timeout = 100;
    // init dap state with fake values
    dap_state.select = 0xffffffff;
    dap_state.csw    = 0xffffffff;
    swd_init();

    // call a target dependant function
    // this function can do several stuff before really initing the debug
    // target_before_init_debug();

    if (JTAG2SWD() != 0) {
        return -1;
    }

    if (swd_write_dp(DP_ABORT, STKCMPCLR | STKERRCLR | WDERRCLR | ORUNERRCLR) != 0) {
        return -1;
    }

    // Ensure CTRL/STAT register selected in DPBANKSEL
    if (swd_write_dp(DP_SELECT, 0) != 0) {
        return -1;
    }

    // Power up
    if (swd_write_dp(DP_CTRL_STAT, CSYSPWRUPREQ | CDBGPWRUPREQ) != 0) {
        return -1;
    }

    for (i = 0; i < timeout; i++) {
        if (swd_read_dp(DP_CTRL_STAT, &tmp) != 0) {
            return -1;
        }

        if ((tmp & (CDBGPWRUPACK | CSYSPWRUPACK)) == (CDBGPWRUPACK | CSYSPWRUPACK)) {
            // Break from loop if powerup is complete
            break;
        }
    }

    if (i == timeout) {
        // Unable to powerup DP
        return -1;
    }

    if (swd_write_dp(DP_CTRL_STAT, CSYSPWRUPREQ | CDBGPWRUPREQ | TRNNORMAL | MASKLANE) != 0) {
        return -1;
    }

    // call a target dependant function:
    // some target can enter in a lock state, this function can unlock these targets
    // target_unlock_sequence();

    if (swd_write_dp(DP_SELECT, 0) != 0) {
        return -1;
    }

    return 0;
}
/*

__attribute__((weak)) void swd_set_target_reset(uint8_t asserted)
{
    (asserted) ? PIN_nRESET_OUT(0) : PIN_nRESET_OUT(1);
}
*/
/**
 * @brief  设置目标复位状态
 * @note   控制目标设备的复位信号，通过软件复位实现
 * @param  asserted: 1-断言复位, 0-释放复位
 * @retval None
 */
void swd_set_target_reset(uint8_t asserted) {
    /* 本文件中对此函数的使用都是先 asserted=1 调用，延时后 asserted=0 调用，为了只调用一次所以只在第二次调用此函数时执行软件复位 */
    if (asserted == 0) {
        swd_write_word(0xE000ED0C, 0x05FA0004);   // 软件复位
    }
}

/**
 * @brief  设置目标状态（硬件复位版本）
 * @note   根据指定状态配置目标设备的调试状态
 * @param  state: 目标状态
 * @retval 0: 成功, -1: 失败
 */
int8_t swd_set_target_state_hw(TARGET_RESET_STATE state) {
    uint32_t val;
    int8_t   ap_retries = 2;

    /* Calling swd_init prior to entering RUN state causes operations to fail. */
    if (state != RUN) {
        swd_init();
    }

    switch (state) {
        case RESET_HOLD:
            swd_set_target_reset(1);
            break;

        case RESET_RUN:
            swd_set_target_reset(1);
            delaymS(20);
            swd_set_target_reset(0);
            delaymS(20);
            swd_off();
            break;

        case RESET_PROGRAM:
            if (swd_init_debug() != 0) {
                return -1;
            }

            // Enable debug
            while (swd_write_word(DBG_HCSR, DBGKEY | C_DEBUGEN) != 0) {
                if (--ap_retries <= 0) {
                    return -1;
                }

                // Target is in invalid state?
                swd_set_target_reset(1);
                delaymS(20);
                swd_set_target_reset(0);
                delaymS(20);
            }

            // Enable halt on reset
            if (swd_write_word(DBG_EMCR, VC_CORERESET) != 0) {
                return -1;
            }

            // Reset again
            swd_set_target_reset(1);
            delaymS(20);
            swd_set_target_reset(0);
            delaymS(20);

            do {
                if (swd_read_word(DBG_HCSR, &val) != 0) {
                    return -1;
                }
            } while ((val & S_HALT) == 0);

            // Disable halt on reset
            if (swd_write_word(DBG_EMCR, 0) != 0) {
                return -1;
            }

            break;

        case NO_DEBUG:
            if (swd_write_word(DBG_HCSR, DBGKEY) != 0) {
                return -1;
            }

            break;

        case DEBUG:
            if (JTAG2SWD() != 0) {
                return -1;
            }

            if (swd_write_dp(DP_ABORT, STKCMPCLR | STKERRCLR | WDERRCLR | ORUNERRCLR) != 0) {
                return -1;
            }

            // Ensure CTRL/STAT register selected in DPBANKSEL
            if (swd_write_dp(DP_SELECT, 0) != 0) {
                return -1;
            }

            // Power up
            if (swd_write_dp(DP_CTRL_STAT, CSYSPWRUPREQ | CDBGPWRUPREQ) != 0) {
                return -1;
            }

            // Enable debug
            if (swd_write_word(DBG_HCSR, DBGKEY | C_DEBUGEN) != 0) {
                return -1;
            }

            break;

        case HALT:
            if (swd_init_debug() != 0) {
                return -1;
            }

            // Enable debug and halt the core (DHCSR <- 0xA05F0003)
            if (swd_write_word(DBG_HCSR, DBGKEY | C_DEBUGEN | C_HALT) != 0) {
                return -1;
            }

            // Wait until core is halted
            do {
                if (swd_read_word(DBG_HCSR, &val) != 0) {
                    return -1;
                }
            } while ((val & S_HALT) == 0);

            break;

        case RUN:
            if (swd_write_word(DBG_HCSR, DBGKEY) != 0) {
                return -1;
            }

            swd_off();

        default:
            return -1;
    }

    return 0;
}

/**
 * @brief  设置目标状态（软件复位版本）
 * @note   根据指定状态配置目标设备的调试状态，使用软件复位
 * @param  state: 目标状态
 * @retval 0: 成功, -1: 失败
 */
int8_t swd_set_target_state_sw(TARGET_RESET_STATE state) {
    uint32_t val;

    /* Calling swd_init prior to enterring RUN state causes operations to fail. */
    if (state != RUN) {
        swd_init();   // 这行不屏蔽的话，无法软件复位程序
    }

    switch (state) {
        case RESET_HOLD:
            swd_set_target_reset(1);
            break;

        case RESET_RUN:
            swd_set_target_reset(1);
            delaymS(20);
            swd_set_target_reset(0);
            delaymS(20);
            swd_off();
            break;

        case RESET_PROGRAM:
            if (swd_init_debug() != 0) {
                return -1;
            }

            // Enable debug and halt the core (DHCSR <- 0xA05F0003)
            if (swd_write_word(DBG_HCSR, DBGKEY | C_DEBUGEN | C_HALT) != 0) {
                return -1;
            }

            // Wait until core is halted
            do {
                if (swd_read_word(DBG_HCSR, &val) != 0) {
                    return -1;
                }
            } while ((val & S_HALT) == 0);

            // Enable halt on reset
            if (swd_write_word(DBG_EMCR, VC_CORERESET) != 0) {
                return -1;
            }

            // Perform a soft reset
            if (swd_read_word(NVIC_AIRCR, &val) != 0) {
                return -1;
            }

            if (swd_write_word(NVIC_AIRCR, VECTKEY | (val & SCB_AIRCR_PRIGROUP_Msk) | SYSRESETREQ) != 0) {
                return -1;
            }

            delaymS(20);

            do {
                if (swd_read_word(DBG_HCSR, &val) != 0) {
                    return -1;
                }
            } while ((val & S_HALT) == 0);

            // Disable halt on reset
            if (swd_write_word(DBG_EMCR, 0) != 0) {
                return -1;
            }

            break;

        case NO_DEBUG:
            if (swd_write_word(DBG_HCSR, DBGKEY) != 0) {
                return -1;
            }

            break;

        case DEBUG:
            if (JTAG2SWD() != 0) {
                return -1;
            }

            if (swd_write_dp(DP_ABORT, STKCMPCLR | STKERRCLR | WDERRCLR | ORUNERRCLR) != 0) {
                return -1;
            }

            // Ensure CTRL/STAT register selected in DPBANKSEL
            if (swd_write_dp(DP_SELECT, 0) != 0) {
                return -1;
            }

            // Power up
            if (swd_write_dp(DP_CTRL_STAT, CSYSPWRUPREQ | CDBGPWRUPREQ) != 0) {
                return -1;
            }

            // Enable debug
            if (swd_write_word(DBG_HCSR, DBGKEY | C_DEBUGEN) != 0) {
                return -1;
            }

            break;

        case HALT:
            if (swd_init_debug() != 0) {
                return -1;
            }

            // Enable debug and halt the core (DHCSR <- 0xA05F0003)
            if (swd_write_word(DBG_HCSR, DBGKEY | C_DEBUGEN | C_HALT) != 0) {
                return -1;
            }

            // Wait until core is halted
            do {
                if (swd_read_word(DBG_HCSR, &val) != 0) {
                    return -1;
                }
            } while ((val & S_HALT) == 0);

            break;

        case RUN:
            if (swd_write_word(DBG_HCSR, DBGKEY) != 0) {
                return -1;
            }

            swd_off();

        default:
            return -1;
    }

    return 0;
}
