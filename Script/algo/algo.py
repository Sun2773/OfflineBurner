# -*- coding: utf-8 -*-
"""
FLM到C文件转换工具
用于将IAR/Keil的FLM（Flash Load Module）文件转换为C语言格式的flash blob文件
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

def parse_flm(flm_path):
    """解析FLM文件，返回各段信息、函数信息和变量信息"""
    if not os.path.exists(flm_path):
        return False, None, None, None, None
    
    try:
        with open(flm_path, 'rb') as f:
            elffile = ELFFile(f)
            
            # 提取所有段
            sections = {}
            section_info = {}
            
            for section in elffile.iter_sections():
                sec_name = section.name
                if sec_name.startswith(('.text', '.data', '.rodata')) or sec_name in ['PrgCode', 'PrgData', 'DevDscr']:
                    sections[sec_name] = section.data()
                    section_info[sec_name] = {
                        'addr': section['sh_addr'],
                        'size': section['sh_size'],
                        'data': section.data()
                    }
            
            # 特别处理PrgCode段
            if 'PrgCode' not in sections:
                for sec_name in sections:
                    if sec_name.startswith('.text'):
                        sections['PrgCode'] = sections[sec_name]
                        section_info['PrgCode'] = section_info[sec_name]
                        break
                else:
                    # 如果没有找到.text段，使用第一个段
                    if sections:
                        first_sec = list(sections.keys())[0]
                        sections['PrgCode'] = sections[first_sec]
                        section_info['PrgCode'] = section_info[first_sec]
                    else:
                        print("Warning: No suitable sections found for PrgCode")
                        sections['PrgCode'] = b''
                        section_info['PrgCode'] = {'addr': 0, 'size': 0, 'data': b''}
            
            # 提取函数信息
            functions = []
            function_map = {}
            
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
            
            # 提取变量信息
            variables = []
            variable_map = {}
            
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
            
            return True, sections, section_info, function_map, variable_map
            
    except Exception as e:
        print(f"Error parsing FLM file {flm_path}: {e}")
        return False, None, None, None, None

def print_analysis_results(functions, variables):
    """输出分析结果"""
    print("\n=== Functions Found ===")
    for func in functions.values():
        print(f"  {func['name']:20} @ 0x{func['addr']:08X} (size: {func['size']:4d} bytes)")
    
    print(f"\n=== Variables Found ({len(variables)} total) ===")
    for var in variables.values():
        data_preview = _format_data_preview(var.get('data'), var['size'])
        print(f"  {var['name']:20} @ 0x{var['addr']:08X} (size: {var['size']:4d} bytes){data_preview}")

def parse_flash_device(variables):
    """解析FlashDevice变量以获取扇区信息"""
    if 'FlashDevice' not in variables:
        print("Warning: FlashDevice variable not found!")
        return []
    
    device = variables['FlashDevice']
    data = device.get('data')
    
    if not data or len(data) < 160:
        print("Warning: FlashDevice data insufficient!")
        return []
    
    # 解析FlashDevice结构
    # 结构体布局：
    # 0-2: Version (2 bytes)
    # 2-130: DevName (128 bytes)
    # 130-132: DevType (2 bytes)
    # 132-136: DevAdr (4 bytes)
    # 136-140: szDev (4 bytes)
    # 140-144: szPage (4 bytes)
    # 144-148: Res (4 bytes)
    # 148-149: valEmpty (1 byte)
    # 149-152: padding (3 bytes)
    # 152-156: toProg (4 bytes)
    # 156-160: toErase (4 bytes)
    # 160+: sectors (8 bytes each: size+addr)
    
    sectors = []
    sector_offset = 160  # 扇区信息从160字节开始
    
    while sector_offset + 8 <= len(data):
        size_bytes = data[sector_offset:sector_offset+4]
        addr_bytes = data[sector_offset+4:sector_offset+8]
        
        if len(size_bytes) < 4 or len(addr_bytes) < 4:
            break
            
        size = struct.unpack('<I', size_bytes)[0]
        addr = struct.unpack('<I', addr_bytes)[0]
        
        if size == 0xFFFFFFFF and addr == 0xFFFFFFFF:
            break
        
        # 只添加有效的扇区（size > 0）
        if size > 0:
            sectors.append({
                'size': size,
                'addr': addr
            })
        
        sector_offset += 8
    
    return sectors

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

def calculate_function_addresses(functions, memory_layout):
    """计算函数在内存中的地址"""
    func_addrs = {}
    
    # 查找关键函数
    for func_name, func_info in functions.items():
        if func_info['section'] == 'PrgCode' or func_info['section'] and func_info['section'].startswith('.text'):
            # 函数在代码段中的相对地址
            relative_addr = func_info['addr']
            # 计算在内存中的绝对地址（+4是因为有开始标记）
            absolute_addr = memory_layout['algo_start'] + 4 + relative_addr
            func_addrs[func_name] = absolute_addr
    
    return func_addrs

def load_template(template_file):
    """加载模板文件"""
    try:
        with open(template_file, 'r', encoding='utf-8') as f:
            return f.read()
    except FileNotFoundError:
        print(f"Error: Template file '{template_file}' not found!")
        sys.exit(1)

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
    
    return '\n'.join(lines)

def generate_sector_info_data(sector_list):
    """生成sector_info数组的数据部分"""
    lines = []
    for sector in sector_list:
        lines.append(f'    {{0x{sector["size"]:04X}, 0x{sector["addr"]:06X}}},')
    return '\n'.join(lines)

def generate_function_comments_data(functions):
    """生成函数注释数据"""
    if not functions:
        return ""
        
    lines = []
    for func in functions.values():
        rel_offset = func['addr'] + 4  # 加4因为有标记
        lines.append(f'//   {func["name"]:12} @ +0x{rel_offset:04X} (size: {func["size"]} bytes)')
    
    return '\n'.join(lines)

def generate_flash_blob_c_from_template(sections, section_info, functions, variables, output_file, chip_name):
    """使用模板生成Flash blob C文件"""
    # 计算动态内存布局
    memory_layout = calculate_memory_layout(len(sections['PrgCode']))
    
    # 解析扇区信息
    sector_list = parse_flash_device(variables)
    
    # 计算函数地址
    func_addrs = calculate_function_addresses(functions, memory_layout)
    
    # 加载模板文件
    template_file = 'flash_blob.tmpl'
    template_content = load_template(template_file)
    
    # 准备模板变量
    template_vars = {
        'chip_name': chip_name,
        'chip_name_lower': chip_name.lower(),
        'algo_start': memory_layout['algo_start'],
        'prog_buffer_addr': memory_layout['prog_buffer_addr'],
        'static_data_addr': memory_layout['static_data_addr'],
        'stack_addr': memory_layout['stack_addr'],
        'algo_start_marker': ALGO_START_MARKER,
        'prog_buffer_size': PROG_BUFFER_SIZE,
        'function_comments': generate_function_comments_data(functions),
        'flash_code_data': generate_flash_code_data(sections['PrgCode']),
        'sector_info_data': generate_sector_info_data(sector_list),
        'init_addr': func_addrs.get('Init') or memory_layout['algo_start'] + 1,
        'uninit_addr': func_addrs.get('UnInit') or memory_layout['algo_start'] + 1,
        'erase_chip_addr': func_addrs.get('EraseChip') or memory_layout['algo_start'] + 1,
        'erase_sector_addr': func_addrs.get('EraseSector') or memory_layout['algo_start'] + 1,
        'program_page_addr': func_addrs.get('ProgramPage') or memory_layout['algo_start'] + 1,
        'bkpt_addr': memory_layout['algo_start'] + 1,
        'rsb_addr': memory_layout['static_data_addr'],
        'rsp_addr': memory_layout['stack_addr'],
    }
    
    # 替换模板变量
    try:
        content = template_content.format(**template_vars)
    except KeyError as e:
        print(f"Error: Missing template variable {e}")
        return False
    
    # 写入文件
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(content)
    
    print(f"Generated flash blob file: {output_file}")
    print(f"Memory layout:")
    print(f"  Algorithm code: 0x{memory_layout['algo_start']:08X} - 0x{memory_layout['algo_end']:08X} ({memory_layout['code_size']+8} bytes)")
    print(f"  Program buffer: 0x{memory_layout['prog_buffer_addr']:08X} - 0x{memory_layout['prog_buffer_addr'] + PROG_BUFFER_SIZE:08X} ({PROG_BUFFER_SIZE} bytes)")
    print(f"  Static data:    0x{memory_layout['static_data_addr']:08X} - 0x{memory_layout['static_data_addr'] + STATIC_DATA_SIZE:08X} ({STATIC_DATA_SIZE} bytes)")
    print(f"  Stack pointer:  0x{memory_layout['stack_addr']:08X}")
    
    # 检查是否有未找到的函数
    for name in ['Init', 'UnInit', 'EraseChip', 'EraseSector', 'ProgramPage']:
        if func_addrs.get(name) is None:
            print(f"Warning: Function '{name}' not found, using default address 0x{memory_layout['algo_start'] + 1:08X}")
    
    return True

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
    
    sections, section_info, functions, variables = result[1:]
    
    # 打印分析结果
    print_analysis_results(functions, variables)
    
    # 生成C文件
    if generate_flash_blob_c_from_template(sections, section_info, functions, variables, output_c_file, chip_name):
        print(f"✓ {flm_path} converted successfully!")
        return True
    else:
        print(f"✗ Failed to convert {flm_path}!")
        return False

def get_user_file_selection(flm_files):
    """获取用户的文件选择"""
    if len(flm_files) == 1:
        return False, flm_files
    
    print("Available FLM files:")
    print(f"  0.\tProcess all files")
    for i, flm in enumerate(flm_files):
        print(f"  {i+1}.\t{flm}")
    
    try:
        choice = int(input("Select FLM file (number): "))
        if choice == 0:
            return True, flm_files  # 处理所有文件
        elif 1 <= choice <= len(flm_files):
            return False, [flm_files[choice-1]]  # 处理单个文件
        else:
            print("Invalid choice, using first file")
            return False, [flm_files[0]]
    except ValueError:
        print("Invalid input, using first file")
        return False, [flm_files[0]]

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

if __name__ == "__main__":
    main()
