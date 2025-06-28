"""
FLM Flash Algorithm Converter

将ARM的FLM（Flash Load Module）文件转换为C语言格式的flash blob文件。
提取Flash编程算法代码、设备信息和扇区配置。

作者: GitHub Copilot
日期: 2025-06-27
"""

import sys
import struct
import os
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection

# 常量定义
SRAM_BASE = 0x20000000
PROG_BUFFER_ADDR = 0x20000400
STATIC_DATA_ADDR = 0x20000C00
STACK_ADDR = 0x20001000
PROG_BUFFER_SIZE = 0x00000400
ALGO_START_MARKER = 0xE00ABE00
ALGO_END_MARKER = 0x00000000

def parse_functions(elf, section_info):
    """解析ELF文件中的函数符号"""
    functions = []
    
    for section in elf.iter_sections():
        if not isinstance(section, SymbolTableSection):
            continue
            
        for symbol in section.iter_symbols():
            if symbol['st_info']['type'] != 'STT_FUNC':
                continue
                
            func_info = {
                'name': symbol.name,
                'addr': symbol['st_value'],
                'size': symbol['st_size'],
                'section': _find_symbol_section(symbol, section_info)
            }
            
            if func_info['section']:
                functions.append(func_info)
    
    return sorted(functions, key=lambda x: x['addr'])

def parse_variables(elf, section_info):
    """解析ELF文件中的全局变量符号"""
    variables = []
    
    for section in elf.iter_sections():
        if not isinstance(section, SymbolTableSection):
            continue
            
        for symbol in section.iter_symbols():
            # 检查是否是对象符号或有大小的NOTYPE符号
            symbol_type = symbol['st_info']['type']
            if not (symbol_type == 'STT_OBJECT' or 
                   (symbol_type == 'STT_NOTYPE' and symbol['st_size'] > 0)):
                continue
                
            var_info = {
                'name': symbol.name,
                'addr': symbol['st_value'],
                'size': symbol['st_size'],
                'type': symbol_type,
                'section': _find_symbol_section(symbol, section_info),
                'data': None
            }
            
            if var_info['section'] and var_info['name']:
                var_info['data'] = _extract_variable_data(var_info, section_info)
                variables.append(var_info)
    
    return sorted(variables, key=lambda x: x['addr'])

def parse_flm(flm_path):
    """解析FLM文件，提取所有必要的段和符号信息"""
    with open(flm_path, 'rb') as f:
        elf = ELFFile(f)
        
        # 提取所需的段
        sections = {'PrgCode': None, 'PrgData': None, 'DevDscr': None}
        section_info = {}
        
        for section in elf.iter_sections():
            sec_name = section.name.strip()
            if sec_name in sections:
                sections[sec_name] = section.data()
                section_info[sec_name] = {
                    'offset': section['sh_offset'],
                    'addr': section['sh_addr'],
                    'size': section['sh_size'],
                    'data': section.data()
                }
        
        # 检查是否所有必需的段都存在
        if None in sections.values():
            print("Error: Missing required sections in FLM file!")
            return None, None, None, None
        
        # 解析符号信息
        functions = parse_functions(elf, section_info)
        variables = parse_variables(elf, section_info)
        
        return sections, section_info, functions, variables

def print_analysis_results(functions, variables):
    """打印解析结果"""
    if functions:
        print(f"Found {len(functions)} functions:")
        for func in functions:
            print(f"  {func['name']} @ 0x{func['addr']:08X} in {func['section']} (size: {func['size']} bytes)")
    else:
        print("No functions found!")
    
    if variables:
        print(f"Found {len(variables)} variables:")
        for var in variables:
            data_preview = _format_data_preview(var['data'], var['size'])
            print(f"  {var['name']} @ 0x{var['addr']:08X} in {var['section']} (size: {var['size']} bytes){data_preview}")
    else:
        print("No variables found!")

def parse_flash_device(variables):
    """解析FlashDevice结构体，提取设备信息和扇区配置"""
    # 查找FlashDevice变量
    flash_device = None
    for var in variables:
        if 'FlashDevice' in var['name'] or var['name'] == 'FlashDevice':
            flash_device = var
            break
    
    if not flash_device or not flash_device['data']:
        print("Warning: FlashDevice structure not found")
        return []
    
    data = flash_device['data']
    if len(data) < 160:
        print("Warning: FlashDevice structure too small")
        return []
    
    try:
        # 解析FlashDevice基本信息
        dev_name = data[2:130].rstrip(b'\x00').decode('ascii', errors='ignore')
        dev_type = struct.unpack('<H', data[130:132])[0]
        dev_addr = struct.unpack('<I', data[132:136])[0]
        sz_dev = struct.unpack('<I', data[136:140])[0]
        sz_page = struct.unpack('<I', data[140:144])[0]
        
        print(f"FlashDevice Info:")
        print(f"  Name: {dev_name}")
        print(f"  Type: {dev_type}, Address: 0x{dev_addr:08X}")
        print(f"  Size: {sz_dev} bytes, Page: {sz_page} bytes")
        
        # 解析扇区信息（从偏移160开始）
        sectors_data = data[160:]
        sector_info = []
        
        for offset in range(0, len(sectors_data), 8):
            if offset + 8 > len(sectors_data):
                break
                
            sz_sector = struct.unpack('<I', sectors_data[offset:offset+4])[0]
            addr_sector = struct.unpack('<I', sectors_data[offset+4:offset+8])[0]
            
            # 检查结束标记
            if sz_sector == 0xFFFFFFFF and addr_sector == 0xFFFFFFFF:
                break
                
            if sz_sector > 0:
                sector_info.append({'size': sz_sector, 'addr': addr_sector})
        
        print(f"Found {len(sector_info)} sectors:")
        for i, sector in enumerate(sector_info):
            print(f"  Sector {i}: 0x{sector['size']:04X} bytes @ 0x{sector['addr']:06X}")
            
        return sector_info
        
    except Exception as e:
        print(f"Error parsing FlashDevice: {e}")
        return []

def calculate_function_addresses(functions):
    """计算函数的运行时地址"""
    func_addrs = {
        'Init': None, 'UnInit': None, 'EraseChip': None,
        'EraseSector': None, 'ProgramPage': None
    }
    
    for func in functions:
        if func['name'] in func_addrs:
            # 运行时地址 = SRAM基址 + 4字节偏移（标记） + 函数偏移 + 1（Thumb模式）
            runtime_addr = SRAM_BASE + 4 + func['addr']
            func_addrs[func['name']] = runtime_addr | 1
    
    return func_addrs

def generate_sram_layout_comment(chip_name):
    """生成SRAM布局注释"""
    return f'''/*
 * {chip_name} SRAM布局 (基址0x{SRAM_BASE:08X}):
 *
 * 0x{SRAM_BASE:08X} ┌─────────────────┐
 *            │ Flash Algorithm │  <- algo_start (算法代码)
 *            │    Code         │
 * 0x{PROG_BUFFER_ADDR:08X} ├─────────────────┤
 *            │ Program Buffer  │  <- program_buffer (数据缓冲区)
 *            │  (1024 bytes)   │
 * 0x{PROG_BUFFER_ADDR + PROG_BUFFER_SIZE:08X} ├─────────────────┤
 *            │                 │
 *            │   Free Space    │
 *            │                 │
 * 0x{STATIC_DATA_ADDR:08X} ├─────────────────┤
 *            │  Static Data    │  <- static_base (全局/静态变量)
 *            │     Area        │
 * 0x{STACK_ADDR:08X} ├─────────────────┤
 *            │     Stack       │  <- stack_pointer (栈空间)
 *            │   (grows down)  │
 *            └─────────────────┘
 */

'''
    
def generate_function_comments(functions):
    """生成函数信息注释"""
    if not functions:
        return "// Flash programming algorithm code\n"
        
    lines = ["// Flash programming algorithm code", "// Functions:"]
    for func in functions:
        rel_offset = func['addr'] + 4  # 加4因为有标记
        lines.append(f'//   {func["name"]:12} @ +0x{rel_offset:04X} (size: {func["size"]} bytes)')
    
    return '\n'.join(lines) + '\n'

def generate_flash_code_array(prgcode_data):
    """生成flash_code数组"""
    lines = [
        "static const uint32_t flash_code[] = {",
        "    // clang-format off",
        f"    0x{ALGO_START_MARKER:08X},                                      // +0x0000"
    ]
    
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
    if lines and ' // ' in lines[-1]:
        code_part, comment_part = lines[-1].split(' // ', 1)
        # 检查最后一行有多少个元素
        last_line_values = code_part.strip().rstrip(',').split(', ')
        # 去掉开头的空格部分，只计算实际的十六进制值
        actual_values = [v for v in last_line_values if v.strip().startswith('0x')]
        
        if len(actual_values) >= 4:
            # 最后一行已经有4个或更多元素，需要换行
            next_offset = len(prgcode_data) + 4  # +4因为有开始标记
            end_marker_line = f'    0x{ALGO_END_MARKER:08X}'
            spaces_needed = max(1, 53 - len(end_marker_line))
            new_line = end_marker_line + ' ' * spaces_needed + f'// +0x{next_offset:04X}'
            lines.append(new_line)
        else:
            # 最后一行元素不足4个，直接添加在末尾
            code_part = code_part.rstrip(' ,') + f', 0x{ALGO_END_MARKER:08X},'
            spaces_needed = max(1, 53 - len(code_part))
            lines[-1] = code_part + ' ' * spaces_needed + '// ' + comment_part
    
    lines.extend(["    // clang-format on", "};", ""])
    return '\n'.join(lines)

def generate_sector_info_array(sector_list):
    """生成sector_info数组"""
    lines = ["// Flash sector information", "static const sector_info_t sector_info[] = {"]
    
    for sector in sector_list:
        lines.append(f'    {{0x{sector["size"]:04X}, 0x{sector["addr"]:06X}}},')
    
    lines.extend(["};", ""])
    return '\n'.join(lines)

def generate_program_target(chip_name, func_addrs):
    """生成program_target_t结构"""
    lines = [
        "// Flash programming target configuration",
        f"const program_target_t _{chip_name.lower()}_ = {{"
    ]
    
    # 函数地址
    func_names = ['Init', 'UnInit', 'EraseChip', 'EraseSector', 'ProgramPage']
    for name in func_names:
        addr = func_addrs.get(name) or SRAM_BASE + 1
        lines.append(f"    0x{addr:08X},   // {name}")
        if func_addrs.get(name) is None:
            print(f"Warning: Function '{name}' not found, using default address 0x{addr:08X}")
    
    # 寄存器设置
    lines.extend([
        "    {",
        f"        0x{SRAM_BASE + 1:08X},   // BKPT : 断点地址 (算法起始+1，Thumb模式)",
        f"        0x{STATIC_DATA_ADDR:08X},   // RSB  : 静态数据基址",
        f"        0x{STACK_ADDR:08X},   // RSP  : 栈指针地址",
        "    },",
    ])
    
    # 其他参数
    lines.extend([
        f"    0x{PROG_BUFFER_ADDR:08X},           // 编程缓冲区地址",
        f"    0x{SRAM_BASE:08X},           // 算法代码起始地址",
        "    sizeof(flash_code),   // 算法代码大小",
        "    flash_code,           // 算法代码数据指针",
        f"    0x{PROG_BUFFER_SIZE:08X},           // 编程缓冲区大小",
        "",
        "    sector_info,                                    // 扇区信息指针",
        "    sizeof(sector_info) / sizeof(sector_info[0]),   // 扇区数量",
        "};"
    ])
    
    return '\n'.join(lines)

def generate_flash_blob_c(sections, section_info, functions, variables, output_file, chip_name):
    """生成Flash blob C文件"""
    # 解析扇区信息
    sector_list = parse_flash_device(variables)
    
    # 计算函数地址
    func_addrs = calculate_function_addresses(functions)
    
    # 生成C文件内容
    content_parts = [
        '#include "flash_blob.h"\n',
        generate_sram_layout_comment(chip_name),
        generate_function_comments(functions),
        generate_flash_code_array(sections['PrgCode']),
        generate_sector_info_array(sector_list),
        generate_program_target(chip_name, func_addrs)
    ]
    
    # 写入文件
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write('\n'.join(content_parts))
    
    print(f"Generated flash blob file: {output_file}")
    return True
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
    
    if rel_offset + var_info['size'] <= len(info['data']):
        return info['data'][rel_offset:rel_offset + var_info['size']]
    return None

def _format_data_preview(data, size):
    """格式化数据预览字符串"""
    if not data or len(data) == 0:
        return ""
        
    if size <= 4:
        if size == 1:
            return f" = 0x{data[0]:02X}"
        elif size == 2:
            val = struct.unpack('<H', data)[0]
            return f" = 0x{val:04X}"
        elif size == 4:
            val = struct.unpack('<I', data)[0]
            return f" = 0x{val:08X}"
    else:
        preview_bytes = data[:min(8, len(data))]
        hex_str = ' '.join([f"{b:02X}" for b in preview_bytes])
        return f" = [{hex_str}...]"
    
    return ""

def get_user_file_selection(flm_files):
    """获取用户的文件选择"""
    if len(flm_files) == 1:
        return False, flm_files
    
    print("Available FLM files:")
    for i, flm in enumerate(flm_files):
        print(f"  {i+1}.\t{flm}")
    print(f"  {len(flm_files)+1}.\tProcess all files")
    
    try:
        choice = int(input("Select FLM file (number): ")) - 1
        if choice == len(flm_files):
            return True, flm_files  # 处理所有文件
        elif 0 <= choice < len(flm_files):
            return False, [flm_files[choice]]  # 处理单个文件
        else:
            print("Invalid choice, using first file")
            return False, [flm_files[0]]
    except ValueError:
        print("Invalid input, using first file")
        return False, [flm_files[0]]

def process_single_flm(flm_path):
    """处理单个FLM文件"""
    chip_name = os.path.splitext(flm_path)[0]
    output_c_file = f"{chip_name}.c"
    
    print(f"\nProcessing FLM file: {flm_path}")
    print(f"Output file: {output_c_file}")
    
    result = parse_flm(flm_path)
    if not result[0]:
        print(f"✗ Failed to parse {flm_path}!")
        return False
    
    sections, section_info, functions, variables = result
    
    # 打印分析结果
    print_analysis_results(functions, variables)
    
    # 生成C文件
    if generate_flash_blob_c(sections, section_info, functions, variables, output_c_file, chip_name):
        print(f"✓ {flm_path} converted successfully!")
        return True
    else:
        print(f"✗ Failed to convert {flm_path}!")
        return False

def main():
    """主程序入口"""
    # 检查可用的FLM文件
    flm_files = [f for f in os.listdir('.') if f.endswith('.FLM')]
    
    if not flm_files:
        print("No FLM files found in current directory!")
        sys.exit(1)
    
    # 获取用户选择
    process_all, files_to_process = get_user_file_selection(flm_files)
    
    # 处理选定的文件
    success_count = 0
    for flm_path in files_to_process:
        if process_single_flm(flm_path):
            success_count += 1
    
    # 输出处理结果
    if process_all:
        print(f"\nCompleted processing {len(files_to_process)} files! ({success_count} successful)")
    else:
        print(f"\nProcessing completed! ({success_count}/{len(files_to_process)} successful)")

if __name__ == "__main__":
    main()