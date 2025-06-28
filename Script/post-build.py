# -*- coding: UTF-8 -*- 
"""
后期构建脚本 - 处理编译后的固件文件

该脚本用于处理编译后的固件，包括生成bin文件、合并bootloader、
记录版本信息等操作。
"""
import subprocess
import os
import sys
import shutil
from typing import Tuple, Optional

# 常量定义
CURRENT_PATH = os.path.dirname(os.path.abspath(__file__))
PROJ_DIR = os.path.dirname(CURRENT_PATH)
OUTPUT_DIR = ""
BOOTLOADER_PATH = "Bootloader/Burner-Boot.bin"
OBJCOPY_EXE_PATH = os.path.join(CURRENT_PATH, 'arm-none-eabi-objcopy.exe')
PROJECT_KEY = "PROJECT_NAME "
VERSION_KEY = "SYSTEM_VERSION "
BIN_OFFSET = 0x2000
FILE_SIZE_OFFSET = 0x01C4

def has_git_changes() -> bool:
    """
    检查Git仓库是否有文件变更
    
    Returns:
        bool: 如有文件变更返回True，否则返回False
    """
    try:
        # 检查暂存区和工作区是否有变更
        result = subprocess.run(
            ['git', 'status', '--porcelain'], 
            capture_output=True, 
            text=True, 
            cwd=PROJ_DIR
        )
        
        if result.returncode != 0:
            print("Git command execution failed, assuming changes exist")
            return True
        
        # 如果git status输出为空，表示没有变更
        if not result.stdout.strip():
            print("No file changes in project, skipping file processing")
            return False
        
        print("File changes detected in project, file processing required")
        return True
    except Exception as e:
        print(f"Error checking Git status: {e}, assuming changes exist")
        return True

def has_git_changes_in_directory(directory_path: str) -> bool:
    """
    检查Git仓库中特定目录是否有文件变更
    
    Args:
        directory_path: 要检查的目录路径（相对于仓库根目录）
        
    Returns:
        bool: 如有文件变更返回True，否则返回False
    """
    try:
        # 构建完整路径
        full_path = os.path.join(PROJ_DIR, directory_path)
        
        # 检查目录是否存在
        if not os.path.exists(full_path):
            print(f"Directory {directory_path} does not exist, skipping check")
            return False
        
        # 检查指定目录下的暂存区和工作区是否有变更
        result = subprocess.run(
            ['git', 'status', '--porcelain', directory_path], 
            capture_output=True, 
            text=True, 
            cwd=PROJ_DIR
        )
        
        if result.returncode != 0:
            print(f"Git command execution failed for directory {directory_path}, assuming changes exist")
            return True
        
        # 如果git status输出为空，表示没有变更
        if not result.stdout.strip():
            print(f"No file changes detected in directory: {directory_path}")
            return False
        
        print(f"File changes detected in directory: {directory_path}")
        return True
    except Exception as e:
        print(f"Error checking Git status for directory {directory_path}: {e}, assuming changes exist")
        return True

def extract_project_and_version(src_file: str) -> Tuple[Optional[str], Optional[str]]:
    """
    从源文件中提取工程名和版本号
    
    Args:
        src_file: 源文件路径
        
    Returns:
        Tuple[Optional[str], Optional[str]]: 工程名和版本号元组
    """
    project = None
    version = None
    try:
        with open(src_file, 'r', encoding='gb2312') as file:
            for line in file:
                if project is None and PROJECT_KEY in line: 
                    project = line.split('"')[1]
                elif version is None and VERSION_KEY in line:
                    version = line.split('"')[1]
                if project and version:
                    break
        return project, version
    except Exception as e:
        print(f"Failed to read version information: {e}")
        return None, None


def clear_directory(directory: str) -> None:
    """
    清空指定目录下的文件
    
    Args:
        directory: 要清空的目录路径
    """
    if not os.path.exists(directory):
        os.makedirs(directory)
        print(f"Directory created: {directory}")
        return
        
    for item in os.listdir(directory):
        item_path = os.path.join(directory, item)
        if os.path.isfile(item_path):
            try:
                os.remove(item_path)
            except Exception as e:
                print(f"Failed to delete file {item_path}: {e}")


def copy_file(src_path: str, dest_path: str) -> bool:
    """
    复制文件到指定目录
    
    Args:
        src_path: 源文件路径
        dest_path: 目标文件路径
    
    Returns:
        bool: 复制成功返回True，失败返回False
    """
    try:
        shutil.copy(src_path, dest_path)
        print(f'File copied successfully: {dest_path}')
        return True
    except Exception as e:
        print(f'File copy failed {src_path} -> {dest_path}: {e}')
        return False


def run_objcopy(exe_path: str, out_path: str, bin_path: str) -> bool:
    """
    执行 objcopy 命令生成 bin 文件
    
    Args:
        exe_path: objcopy可执行文件路径
        out_path: .out文件路径
        bin_path: 输出的bin文件路径
        
    Returns:
        bool: 执行成功返回True，失败返回False
    """
    try:
        subprocess.run([exe_path, '-O', 'binary', out_path, bin_path], check=True)
        file_size = os.stat(bin_path).st_size
        print(f'Binary file generated: {os.path.basename(bin_path)} ({file_size} bytes)')
        return True
    except subprocess.CalledProcessError as e:
        print(f'objcopy command execution failed: {e}')
        return False
    except Exception as e:
        print(f'Error generating binary file: {e}')
        return False


def merge_files(bootloader_path: str, bin_path: str, target_path: str, bin_offset: int) -> bool:
    """
    合并 bootloader 和 bin 文件到目标文件
    
    Args:
        bootloader_path: bootloader文件路径
        bin_path: bin文件路径
        target_path: 合并后的目标文件路径
        bin_offset: bin文件的偏移量
        
    Returns:
        bool: 合并成功返回True，失败返回False
    """
    try:
        with open(target_path, 'wb') as f_target:
            # 写入bootloader
            with open(bootloader_path, 'rb') as f_boot:
                boot_data = f_boot.read()
                f_target.write(boot_data)
                boot_size = len(boot_data)
            
            # 填充空白区域
            fill_data = b'\xFF' * (bin_offset - f_target.tell())
            f_target.write(fill_data)
            
            # 写入应用程序
            with open(bin_path, 'rb') as f_bin:
                bin_data = f_bin.read()
                f_target.write(bin_data)
                bin_size = len(bin_data)
            
            # 写入bootloader大小信息
            f_target.seek(FILE_SIZE_OFFSET)
            f_target.write(boot_size.to_bytes(4, byteorder='little'))
            
            # 写入应用程序大小信息
            f_target.seek(bin_offset + FILE_SIZE_OFFSET)
            f_target.write(bin_size.to_bytes(4, byteorder='little'))
        
        total_size = os.stat(target_path).st_size
        print(f'File merge completed: {os.path.basename(target_path)} ({total_size} bytes)')
        return True
    except Exception as e:
        print(f'Error merging files: {e}')
        return False

def main():
    """主函数: 处理编译后的固件文件"""
    global BOOTLOADER_PATH  # 声明使用全局变量
    try:
        # 获取命令行参数
        if len(sys.argv) < 5:
            print("Insufficient parameters: build mode, output path and project name required")
            sys.exit(1)
            
        # 解析参数
        CONFIG_NAME  = sys.argv[1]  # 配置名称
        TARGET_DIR   = sys.argv[2]  # 输出目录
        TARGET_BNAME = sys.argv[3]  # 输出文件名
        PROJ_DIR     = sys.argv[4]  # 项目目录
        PROJ_FNAME   = sys.argv[5]  # 项目名        
        OUTPUT_DIR = os.path.join(PROJ_DIR, '..', 'Output')

        # 提取版本信息
        VERSION_FILE_PATH = os.path.join(PROJ_DIR, '..', 'Version.h')
        project, version = extract_project_and_version(VERSION_FILE_PATH)
        if not project or not version:
            print("Unable to get project name or version number")
            sys.exit(1)
            
        # 生成文件路径
        elf_file_path = os.path.join(TARGET_DIR, f'{TARGET_BNAME}.out')
    
        if CONFIG_NAME == 'Bootloader':
            file_suffix = f'-Boot'
        elif CONFIG_NAME == 'Debug':
            file_suffix = f'-{version}-Alpha'
        else:
            file_suffix = f'-{version}'
        
        bin_file_path = os.path.join(OUTPUT_DIR, CONFIG_NAME, f'{project}{file_suffix}.bin')
        out_file_path = os.path.join(OUTPUT_DIR, CONFIG_NAME, f'{project}{file_suffix}.out')

        # 检查是否需要处理文件
        needs_processing = (
            has_git_changes_in_directory(os.path.join(PROJ_DIR, '..')) or 
            not os.path.exists(out_file_path) or 
            not os.path.exists(bin_file_path)
        )

        if needs_processing:
            print("Starting firmware file processing...")
            clear_directory(os.path.join(OUTPUT_DIR, CONFIG_NAME))
            
            if not copy_file(elf_file_path, out_file_path):
                sys.exit(1)
                
            if not run_objcopy(OBJCOPY_EXE_PATH, out_file_path, bin_file_path):
                sys.exit(1)

            if CONFIG_NAME == "Bootloader":
                # Bootloader 文件处理
                BOOTLOADER_PATH = os.path.join(CONFIG_NAME, f'{project}{file_suffix}.bin').replace('\\', '/')
                # 将新生成的文件路径保存到本脚本文件
                path = os.path.abspath(__file__)
                with open(path, 'r+', encoding='utf-8') as f:
                    lines = f.readlines()
                    for i, line in enumerate(lines):
                        if 'BOOTLOADER_PATH = ' in line:
                            lines[i] = f'BOOTLOADER_PATH = "{BOOTLOADER_PATH}"\n'
                            break
                    f.seek(0)
                    f.writelines(lines)
                    f.truncate()
                print(f"Bootloder Update: {BOOTLOADER_PATH}")
            else:
                # 应用程序文件处理，合并 bootloader 和 bin 文件
                BOOTLOADER_PATH = os.path.join(OUTPUT_DIR, BOOTLOADER_PATH)
                target_file_path = os.path.join(OUTPUT_DIR, CONFIG_NAME, f'{project}{file_suffix}+boot.bin')
                if os.path.exists(BOOTLOADER_PATH):
                    if not merge_files(BOOTLOADER_PATH, bin_file_path, target_file_path, BIN_OFFSET):
                        sys.exit(1)
                    print(f"Bootloader merged with application: {os.path.basename(target_file_path)}")
                else:
                    print(f"Bootloader file does not exist: {BOOTLOADER_PATH}")
                pass

        else:
            print("Skipping file processing")
    
            
        with open(os.path.join(PROJ_DIR, f'{PROJ_FNAME}.jdebug'), "r+") as f:
            lines = f.readlines()
            for i, line in enumerate(lines):
                if 'File.Open' in line:
                    lines[i] = f'  File.Open ("$(ProjectDir)/../../Output/{project}{file_suffix}.out");\n'
                    break
            f.seek(0)
            f.writelines(lines)
            f.truncate()
        
    except Exception as e:
        print(f"Error occurred during processing: {e}")
        sys.exit(1)


if __name__ == '__main__':
    main()