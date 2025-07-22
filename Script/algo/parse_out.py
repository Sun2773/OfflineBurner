# -*- coding: utf-8 -*-
"""
OUT文件到C文件转换工具
用于将IAR的.out文件转换为C语言格式的flash blob文件
基于algo.py的功能改写，支持解析IAR输出的ELF格式.out文件
"""

import struct
import os
import sys
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection

# 内存布局配置
ALGO_START_MARKER = 0xE00ABE00  # 算法开始标记
ALGO_END_MARKER   = 0x00000000  # 算法结束标记
PROG_BUFFER_SIZE  = 0x00000400  # 1K编程缓冲区
STATIC_DATA_SIZE  = 0x00000400  # 1K字节静态数据区

def parse_out_file(out_path):
    """解析.out文件，返回各段信息、函数信息和变量信息"""
    if not os.path.exists(out_path):
        print(f"Error: File {out_path} not found!")
        return False, None, None, None, None
    
    try:
        with open(out_path, 'rb') as f:
            elffile = ELFFile(f)
            
            print(f"ELF file info:")
            print(f"  Class: {elffile.get_machine_arch()}")
            print(f"  Data: {elffile.header['e_ident']['EI_DATA']}")
            print(f"  Machine: {elffile.header['e_machine']}")
            print(f"  Entry point: 0x{elffile.header['e_entry']:08X}")
            
            # 提取所有段
            sections = {}
            section_info = {}
            
            print(f"\nSections found:")
            for section in elffile.iter_sections():
                sec_name = section.name
                sec_size = section['sh_size']
                sec_addr = section['sh_addr']
                sec_type = section['sh_type']
                
                print(f"  {sec_name:20} @ 0x{sec_addr:08X} (size: {sec_size:6d} bytes, type: {sec_type})")
                
                # 只保存有数据的段
                if sec_size > 0 and section.data():
                    sections[sec_name] = section.data()
                    section_info[sec_name] = {
                        'addr': sec_addr,
                        'size': sec_size,
                        'data': section.data(),
                        'type': sec_type
                    }
            
            # 查找代码段 - IAR的.out文件通常使用P1段作为代码段
            code_section = None
            code_section_name = None
            
            # 优先查找P1段（IAR常用）
            if 'P1' in sections:
                code_section = sections['P1']
                code_section_name = 'P1'
                print(f"\nUsing P1 section as code section (IAR format)")
            # 查找.text段
            elif '.text' in sections:
                code_section = sections['.text']
                code_section_name = '.text'
                print(f"\nUsing .text section as code section")
            else:
                # 查找其他可能的代码段 - 有地址且大于0的PROGBITS段
                for sec_name, info in section_info.items():
                    if (info['type'] == 'SHT_PROGBITS' and 
                        info['addr'] > 0 and 
                        info['size'] > 100 and  # 代码段通常比较大
                        not sec_name.startswith('.debug')):  # 排除调试段
                        code_section = sections[sec_name]
                        code_section_name = sec_name
                        print(f"\nUsing {sec_name} section as code section")
                        break
            
            if not code_section:
                print("Warning: No suitable code section found!")
                code_section = b''
                code_section_name = 'unknown'
            
            # 将代码段存储为PrgCode（保持与algo.py兼容）
            sections['PrgCode'] = code_section
            section_info['PrgCode'] = section_info.get(code_section_name, {'addr': 0, 'size': 0, 'data': b''})
            
            # 提取函数信息
            functions = []
            function_map = {}
            
            print(f"\nFunctions found:")
            for section in elffile.iter_sections():
                if isinstance(section, SymbolTableSection):
                    for symbol in section.iter_symbols():
                        if symbol['st_info']['type'] == 'STT_FUNC' and symbol['st_size'] > 0:
                            func_info = {
                                'name': symbol.name,
                                'addr': symbol['st_value'],
                                'size': symbol['st_size'],
                                'section': _find_symbol_section(symbol, section_info)
                            }
                            functions.append(func_info)
                            function_map[symbol.name] = func_info
                            print(f"  {symbol.name:20} @ 0x{symbol['st_value']:08X} (size: {symbol['st_size']:4d} bytes)")
            
            # 尝试提取实际的代码部分（排除常量数据）
            if function_map:
                # 找到最小的函数地址作为代码起始点
                min_func_addr = min(func_info['addr'] for func_info in function_map.values())
                
                # ARM Thumb指令集：函数地址为奇数，实际代码地址为偶数（函数地址-1）
                actual_code_start = min_func_addr & ~1  # 清除最低位，得到实际代码起始地址
                
                code_base = section_info['PrgCode']['addr']
                code_start_offset = actual_code_start - code_base
                
                # 从实际代码起始地址开始提取到段的结束
                if code_start_offset >= 0 and code_start_offset < len(code_section):
                    # 提取从实际代码起始到段结束的所有代码
                    actual_code = code_section[code_start_offset:]
                    
                    # 移除尾部的填充零（如果有的话）
                    while len(actual_code) > 4 and actual_code[-4:] == b'\x00\x00\x00\x00':
                        actual_code = actual_code[:-4]
                    
                    sections['PrgCode'] = actual_code
                    
                    # 更新代码段信息
                    section_info['PrgCode'] = {
                        'addr': actual_code_start,  # 使用实际代码起始地址
                        'size': len(actual_code),
                        'data': actual_code,
                        'type': section_info['PrgCode'].get('type', 'SHT_PROGBITS'),
                        'original_addr': code_base,
                        'code_offset': code_start_offset
                    }
                    
                    print(f"Extracted actual code: {len(actual_code)} bytes from offset 0x{code_start_offset:04X} to end")
                    print(f"First function address: 0x{min_func_addr:08X} (Thumb)")
                    print(f"Actual code starts at: 0x{actual_code_start:08X} (Code)")
                else:
                    print(f"Warning: Invalid code extraction range, using full section")
            
            # 提取变量信息
            variables = []
            variable_map = {}
            
            print(f"\nVariables found:")
            for section in elffile.iter_sections():
                if isinstance(section, SymbolTableSection):
                    for symbol in section.iter_symbols():
                        if symbol['st_info']['type'] == 'STT_OBJECT' and symbol['st_size'] > 0:
                            var_info = {
                                'name': symbol.name,
                                'addr': symbol['st_value'],
                                'size': symbol['st_size'],
                                'section': _find_symbol_section(symbol, section_info)
                            }
                            # 提取变量数据
                            var_data = _extract_variable_data(var_info, section_info)
                            var_info['data'] = var_data
                            variables.append(var_info)
                            variable_map[symbol.name] = var_info
                            
                            data_preview = _format_data_preview(var_data, var_info['size'])
                            print(f"  {symbol.name:20} @ 0x{symbol['st_value']:08X} (size: {symbol['st_size']:4d} bytes){data_preview}")
            
            return True, sections, section_info, function_map, variable_map
            
    except Exception as e:
        print(f"Error parsing OUT file {out_path}: {e}")
        import traceback
        traceback.print_exc()
        return False, None, None, None, None

def calculate_memory_layout(code_size):
    """计算动态内存布局"""
    # 代码段对齐到4字节
    aligned_code_size = (code_size + 3) & ~3
    
    # 算法代码起始地址
    algo_start = 0x20000000
    
    # 算法代码结束地址（包含起始和结束标记）
    algo_end = algo_start + 4 + aligned_code_size + 4
    
    # 编程缓冲区地址：从算法代码结束后开始，1K对齐
    prog_buffer_addr = (algo_end + 1023) & ~1023
    
    # 静态数据区：紧跟编程缓冲区
    static_data_addr = prog_buffer_addr + PROG_BUFFER_SIZE
    
    # 栈地址：紧跟静态数据区
    stack_addr = static_data_addr + STATIC_DATA_SIZE
    
    return {
        'algo_start': algo_start,
        'algo_end': algo_end,
        'prog_buffer_addr': prog_buffer_addr,
        'static_data_addr': static_data_addr,
        'stack_addr': stack_addr,
        'code_size': aligned_code_size
    }

def calculate_function_addresses(functions, memory_layout, code_section_info):
    """计算函数在内存中的地址"""
    func_addrs = {}
    
    # 获取代码段的基地址和大小
    code_base_addr = code_section_info.get('addr', 0)
    code_size = code_section_info.get('size', 0)
    
    # 获取原始基地址和偏移量（如果有的话）
    original_addr = code_section_info.get('original_addr', code_base_addr)
    code_offset = code_section_info.get('code_offset', 0)
    
    print(f"\nCode section info:")
    print(f"  Current base: 0x{code_base_addr:08X}, size: 0x{code_size:04X}")
    if code_offset > 0:
        print(f"  Original base: 0x{original_addr:08X}, code offset: 0x{code_offset:04X}")
    
    # 查找关键函数
    for func_name, func_info in functions.items():
        func_addr = func_info['addr']
        
        # 计算函数相对于代码起始的偏移
        if code_offset > 0:
            # 如果我们提取了代码的一部分，函数地址需要减去代码偏移
            relative_addr = func_addr - code_base_addr
        else:
            # 如果使用完整的代码段，正常计算
            relative_addr = func_addr - code_base_addr
            
        # 计算在内存中的绝对地址（+4是因为有开始标记）
        absolute_addr = memory_layout['algo_start'] + 4 + relative_addr
        func_addrs[func_name] = absolute_addr
        print(f"  {func_name:12}: 0x{func_addr:08X} -> 0x{absolute_addr:08X} (relative: +0x{relative_addr:04X})")
    
    return func_addrs

def generate_flash_code_data(prgcode_data):
    """生成flash_code数组的数据部分"""
    lines = []
    
    # 按16字节一行输出代码
    for i in range(0, len(prgcode_data), 16):
        chunk = prgcode_data[i:i+16]
        # 填充到4字节对齐
        while len(chunk) % 4 != 0:
            chunk += b'\x00'
        
        values = []
        for j in range(0, len(chunk), 4):
            if j + 4 <= len(chunk):
                val = struct.unpack('<I', chunk[j:j+4])[0]
                values.append(f'0x{val:08X}')
        
        code_part = f'    {", ".join(values)},'
        comment_part = f'// +0x{i+4:04X}'
        
        # 对齐注释到第53列
        spaces_needed = max(1, 53 - len(code_part))
        aligned_line = code_part + ' ' * spaces_needed + comment_part
        lines.append(aligned_line)
    
    # 处理结束标记的添加
    if lines:
        # 检查最后一行是否需要添加结束标记
        last_line = lines[-1]
        if ' // ' in last_line:
            code_part, comment_part = last_line.split(' // ', 1)
            # 检查最后一行有多少个元素
            last_line_values = code_part.strip().rstrip(',').split(', ')
            # 去掉开头的空格部分，只计算实际的十六进制值
            actual_values = [v for v in last_line_values if v.strip().startswith('0x')]
            
            if len(actual_values) >= 4:
                # 最后一行已经有4个或更多元素，需要换行
                next_offset = len(prgcode_data) + 4  # +4因为有开始标记
                end_marker_line = f'    0x{ALGO_END_MARKER:08X},'
                spaces_needed = max(1, 53 - len(end_marker_line))
                new_line = end_marker_line + ' ' * spaces_needed + f'// +0x{next_offset:04X}'
                lines.append(new_line)
            else:
                # 最后一行元素不足4个，直接添加在末尾
                code_part = code_part.rstrip(' ,') + f', 0x{ALGO_END_MARKER:08X},'
                spaces_needed = max(1, 53 - len(code_part))
                lines[-1] = code_part + ' ' * spaces_needed + '// ' + comment_part
    else:
        # 如果没有代码，只添加结束标记
        lines.append(f'    0x{ALGO_END_MARKER:08X},                          // +0x0004')
    
    return '\n'.join(lines)

def generate_sector_info_data(sector_list):
    """生成sector_info数组的数据部分"""
    if not sector_list:
        # 如果没有扇区信息，生成默认的
        return '    {0x4000, 0x000000},\n    {0x10000, 0x010000},\n    {0x20000, 0x020000},'
    
    lines = []
    for sector in sector_list:
        lines.append(f'    {{0x{sector["size"]:04X}, 0x{sector["addr"]:06X}}},')
    return '\n'.join(lines)

def generate_function_comments_data(functions, func_addrs, memory_layout):
    """生成函数注释数据"""
    if not functions:
        return ""
        
    lines = []
    for func_name, func_info in functions.items():
        if func_name in func_addrs:
            # 计算相对于flash_code数组开始的偏移
            # func_addrs中的地址是运行时地址，直接减去算法起始地址即可得到数组偏移
            array_offset = func_addrs[func_name] - memory_layout['algo_start']
            lines.append(f'//   {func_name:12} @ +0x{array_offset:04X} (size: {func_info["size"]} bytes)')
    
    return '\n'.join(lines)

def create_default_sector_info():
    """创建默认的扇区信息"""
    return [
        {'size': 0x4000, 'addr': 0x000000},
        {'size': 0x10000, 'addr': 0x010000},
        {'size': 0x20000, 'addr': 0x020000},
    ]

def generate_flash_blob_c(sections, section_info, functions, variables, output_file, chip_name):
    """生成Flash blob C文件"""
    # 计算动态内存布局
    memory_layout = calculate_memory_layout(len(sections['PrgCode']))
    
    # 获取代码段信息
    code_section_info = section_info['PrgCode']
    
    # 计算函数地址
    func_addrs = calculate_function_addresses(functions, memory_layout, code_section_info)
    
    # 创建默认扇区信息（由于.out文件通常不包含扇区信息）
    sector_list = create_default_sector_info()
    
    # 生成C文件内容
    content = f'''#include "flash_blob.h"

/*
 * {chip_name} SRAM布局 (基址0x{memory_layout['algo_start']:08X}):
 *
 * 0x{memory_layout['algo_start']:08X} ┌─────────────────┐
 *            │ Flash Algorithm │  <- algo_start (算法代码)
 *            │    Code         │
 * 0x{memory_layout['prog_buffer_addr']:08X} ├─────────────────┤
 *            │ Program Buffer  │  <- program_buffer (数据缓冲区)
 *            │  (1024 bytes)   │
 * 0x{memory_layout['static_data_addr']:08X} ├─────────────────┤
 *            │  Static Data    │  <- static_base (全局/静态变量)
 *            │     Area        │
 * 0x{memory_layout['stack_addr']:08X} ├─────────────────┤
 *            │     Stack       │  <- stack_pointer (栈空间)
 *            │   (grows down)  │
 *            │                 │
 *            │ ............... │
 *
 */

// Flash programming algorithm code
// Functions:
{generate_function_comments_data(functions, func_addrs, memory_layout)}

static const uint32_t flash_code[] = {{
    // clang-format off
    0x{ALGO_START_MARKER:08X},                                      // +0x0000
{generate_flash_code_data(sections['PrgCode'])}
    // clang-format on
}};

// Flash sector information
static const sector_info_t sector_info[] = {{
{generate_sector_info_data(sector_list)}
}};

// Flash programming target configuration
const program_target_t _{chip_name.lower()}_ = {{
    0x{func_addrs.get('Init', memory_layout['algo_start'] + 1):08X},   // Init
    0x{func_addrs.get('UnInit', memory_layout['algo_start'] + 1):08X},   // UnInit
    0x{func_addrs.get('EraseChip', memory_layout['algo_start'] + 1):08X},   // EraseChip
    0x{func_addrs.get('EraseSector', memory_layout['algo_start'] + 1):08X},   // EraseSector
    0x{func_addrs.get('ProgramPage', memory_layout['algo_start'] + 1):08X},   // ProgramPage
    {{
        0x{memory_layout['algo_start'] + 1:08X},   // BKPT : 断点地址 (算法起始+1，Thumb模式)
        0x{memory_layout['static_data_addr']:08X},   // RSB  : 静态数据基址
        0x{memory_layout['stack_addr']:08X},   // RSP  : 栈指针地址
    }},
    0x{memory_layout['prog_buffer_addr']:08X},           // 编程缓冲区地址
    0x{memory_layout['algo_start']:08X},           // 算法代码起始地址
    sizeof(flash_code),   // 算法代码大小
    flash_code,           // 算法代码数据指针
    0x{PROG_BUFFER_SIZE:08X},           // 编程缓冲区大小

    sector_info,                                    // 扇区信息指针
    sizeof(sector_info) / sizeof(sector_info[0]),   // 扇区数量
}};
'''
    
    # 写入文件
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(content)
    
    print(f"\n✓ Generated flash blob file: {output_file}")
    print(f"Memory layout:")
    print(f"  Algorithm code: 0x{memory_layout['algo_start']:08X} - 0x{memory_layout['algo_end']:08X} ({memory_layout['code_size']+8} bytes)")
    print(f"  Program buffer: 0x{memory_layout['prog_buffer_addr']:08X} - 0x{memory_layout['prog_buffer_addr'] + PROG_BUFFER_SIZE:08X} ({PROG_BUFFER_SIZE} bytes)")
    print(f"  Static data:    0x{memory_layout['static_data_addr']:08X} - 0x{memory_layout['static_data_addr'] + STATIC_DATA_SIZE:08X} ({STATIC_DATA_SIZE} bytes)")
    print(f"  Stack pointer:  0x{memory_layout['stack_addr']:08X}")
    
    print(f"\nFunction addresses:")
    for name in ['Init', 'UnInit', 'EraseChip', 'EraseSector', 'ProgramPage']:
        addr = func_addrs.get(name)
        if addr:
            print(f"  {name:12}: 0x{addr:08X}")
        else:
            print(f"  {name:12}: 0x{memory_layout['algo_start'] + 1:08X} (default)")
    
    return True

def process_out_file(out_path):
    """处理单个.out文件"""
    chip_name = os.path.splitext(os.path.basename(out_path))[0]
    output_c_file = f"{chip_name}.c"
    
    print(f"Processing OUT file: {out_path}")
    print(f"Output file: {output_c_file}")
    print(f"Chip name: {chip_name}")
    
    result = parse_out_file(out_path)
    if not result[0]:
        print(f"✗ Failed to parse {out_path}!")
        return False
    
    sections, section_info, functions, variables = result[1:]
    
    # 生成C文件
    if generate_flash_blob_c(sections, section_info, functions, variables, output_c_file, chip_name):
        print(f"✓ {out_path} converted successfully!")
        return True
    else:
        print(f"✗ Failed to convert {out_path}!")
        return False

def main():
    """主程序入口"""
    if len(sys.argv) != 2:
        print("Usage: python parse_out.py <out_file>")
        print("Example: python parse_out.py STM32F4xx.out")
        sys.exit(1)
    
    out_file = sys.argv[1]
    
    if not os.path.exists(out_file):
        print(f"Error: File {out_file} not found!")
        sys.exit(1)
    
    if not out_file.endswith('.out'):
        print(f"Warning: File {out_file} does not have .out extension")
    
    success = process_out_file(out_file)
    
    if success:
        print(f"\n✓ Conversion completed successfully!")
    else:
        print(f"\n✗ Conversion failed!")
        sys.exit(1)

# 辅助函数
def _find_symbol_section(symbol, section_info):
    """查找符号所属的段"""
    addr = symbol['st_value']
    for sec_name, info in section_info.items():
        if info['addr'] <= addr < info['addr'] + info['size']:
            return sec_name
    return None

def _extract_variable_data(var_info, section_info):
    """提取变量的实际数据"""
    section_name = var_info['section']
    if not section_name or section_name not in section_info:
        return None
        
    info = section_info[section_name]
    rel_offset = var_info['addr'] - info['addr']
    
    if rel_offset >= 0 and rel_offset + var_info['size'] <= len(info['data']):
        return info['data'][rel_offset:rel_offset + var_info['size']]
    return None

def _format_data_preview(data, size):
    """格式化数据预览字符串"""
    if not data or len(data) == 0:
        return ""
        
    if size <= 4:
        if size == 1:
            return f" = 0x{data[0]:02X}"
        elif size == 2 and len(data) >= 2:
            val = struct.unpack('<H', data[:2])[0]
            return f" = 0x{val:04X}"
        elif size == 4 and len(data) >= 4:
            val = struct.unpack('<I', data[:4])[0]
            return f" = 0x{val:08X}"
    else:
        preview_bytes = data[:min(8, len(data))]
        hex_str = ' '.join([f"{b:02X}" for b in preview_bytes])
        return f" = [{hex_str}...]"
    
    return ""

if __name__ == "__main__":
    main()
