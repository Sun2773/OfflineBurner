# -*- coding: UTF-8 -*- 
"""
    该脚本用于在编译前执行与处理项目相关的操作
"""

import subprocess
import os
import sys
from typing import Tuple, Optional

PROJECT_KEY = "PROJECT_NAME "
VERSION_KEY = "SYSTEM_VERSION "

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
        with open(src_file, 'r', encoding='utf-8') as file:
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


def main():
    try:
        # 获取命令行参数
        if len(sys.argv) < 6:  # 修正参数检查：脚本名 + 5个必要参数
            print('Insufficient parameters: configuration name, output directory, output filename, project directory and project name required')
            sys.exit(1)
        
        # 解析参数
        CONFIG_NAME  = sys.argv[1]  # 配置名称
        TARGET_DIR   = sys.argv[2]  # 输出目录
        TARGET_BNAME = sys.argv[3]  # 输出文件名
        PROJ_DIR     = sys.argv[4]  # 项目目录
        PROJ_FNAME   = sys.argv[5]  # 项目名
        
        # 定义输出目录
        OUTPUT_DIR = os.path.join(PROJ_DIR, '..', '..', 'Output')
        os.makedirs(OUTPUT_DIR, exist_ok=True)
        # 提取版本信息
        VERSION_FILE_PATH = os.path.join(PROJ_DIR, '..', 'Version.h')
        project, version = extract_project_and_version(VERSION_FILE_PATH)
        if not project or not version:
            print("Unable to get project name or version number")
            sys.exit(1)
        # 去掉version字符串中的非数字字符
        version = ''.join(filter(str.isdigit, version))
        with open(VERSION_FILE_PATH, "r+", encoding='utf-8') as f:
            lines = f.readlines()
            for i, line in enumerate(lines):
                if 'VersionNumber' in line:
                    lines[i] = f'const uint16_t VersionNumber = {version};\n'
                    break
            f.seek(0)
            f.writelines(lines)
            f.truncate()
    except Exception as e:
        print(f'Error occurred during execution: {e}')
        sys.exit(1)

def run_subprocess(script_path):
    """
    执行子进程并处理可能的错误
    """
    if not os.path.exists(script_path):
        print(f'Script does not exist: {script_path}')
        return
    
    try:
        print(f'Executing script: {script_path}')
        result = subprocess.run(['python', script_path], check=True, capture_output=True, text=True)
        print(f'Script {os.path.basename(script_path)} executed successfully')
    except subprocess.CalledProcessError as e:
        print(f'Script execution failed: {e}')
        print(f'Error output: {e.stderr}')

if __name__ == '__main__':
    main()
