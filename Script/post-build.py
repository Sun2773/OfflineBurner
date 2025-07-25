# -*- coding: UTF-8 -*- 
"""
Post-build script for firmware processing

This script handles firmware files after compilation, including:
- Converting ELF to binary files
- Merging bootloader with application
- Version information management

Features:
- Cross-platform compatibility using pyelftools
- Improved error handling and logging
- JSON-based configuration management
"""
import subprocess
import os
import sys
import shutil
import json
from typing import Tuple, Optional, Dict, Any
from elftools.elf.elffile import ELFFile

# Constants
CURRENT_PATH = os.path.dirname(os.path.abspath(__file__))
PROJ_DIR = os.path.dirname(CURRENT_PATH)
CONFIG_FILE = os.path.join(CURRENT_PATH, 'build_config.json')
DEFAULT_BOOTLOADER_PATH = "Bootloader/Burner-Boot.bin"
PROJECT_KEY = "PROJECT_NAME "
VERSION_KEY = "SYSTEM_VERSION "
BIN_OFFSET = 0x2000

class BuildConfig:
    """Build configuration management"""
    def __init__(self, config_file: str):
        self.config_file = config_file
        self.config = self._load_config()
    
    def _load_config(self) -> Dict[str, Any]:
        """Load configuration file"""
        if os.path.exists(self.config_file):
            try:
                with open(self.config_file, 'r', encoding='utf-8') as f:
                    return json.load(f)
            except Exception as e:
                print(f"Failed to load config: {e}")
        return {'bootloader_path': DEFAULT_BOOTLOADER_PATH}
    
    def save_config(self) -> None:
        """Save configuration file"""
        try:
            with open(self.config_file, 'w', encoding='utf-8') as f:
                json.dump(self.config, f, indent=2, ensure_ascii=False)
        except Exception as e:
            print(f"Failed to save config: {e}")
    
    def get_bootloader_path(self) -> str:
        """Get bootloader path"""
        return self.config.get('bootloader_path', DEFAULT_BOOTLOADER_PATH)
    
    def set_bootloader_path(self, path: str) -> None:
        """Set bootloader path"""
        self.config['bootloader_path'] = path
        self.save_config()

def has_git_changes_in_directory(directory_path: str) -> bool:
    """Check if there are Git changes in specified directory"""
    try:
        result = subprocess.run(
            ['git', 'status', '--porcelain', directory_path], 
            capture_output=True, 
            text=True, 
            cwd=PROJ_DIR
        )
        
        if result.returncode != 0:
            print(f"Git check failed for {directory_path}, assuming changes exist")
            return True
        
        if not result.stdout.strip():
            print(f"No changes detected in: {directory_path}")
            return False
        
        print(f"Changes detected in: {directory_path}")
        return True
    except Exception as e:
        print(f"Git check error for {directory_path}: {e}, assuming changes exist")
        return True

def extract_project_and_version(src_file: str) -> Tuple[Optional[str], Optional[str]]:
    """Extract project name and version from source file"""
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
        print(f"Failed to read version info: {e}")
        return None, None

def clear_directory(directory: str) -> None:
    """Clear all files in specified directory"""
    if not os.path.exists(directory):
        os.makedirs(directory)
        print(f"Created directory: {directory}")
        return
        
    for item in os.listdir(directory):
        item_path = os.path.join(directory, item)
        if os.path.isfile(item_path):
            try:
                os.remove(item_path)
            except Exception as e:
                print(f"Failed to delete file {item_path}: {e}")

def copy_file(src_path: str, dest_path: str) -> bool:
    """Copy file to destination"""
    try:
        shutil.copy(src_path, dest_path)
        print(f'File copied: {dest_path}')
        return True
    except Exception as e:
        print(f'Copy failed {src_path} -> {dest_path}: {e}')
        return False


def elf_to_binary(elf_path: str, bin_path: str) -> bool:
    """Convert ELF file to binary using pyelftools"""
    try:
        with open(elf_path, 'rb') as elf_file:
            elffile = ELFFile(elf_file)
            
            # Find all loadable segments
            loadable_segments = []
            min_addr = float('inf')
            max_addr = 0
            
            for segment in elffile.iter_segments():
                if segment['p_type'] == 'PT_LOAD' and segment['p_filesz'] > 0:
                    loadable_segments.append(segment)
                    min_addr = min(min_addr, segment['p_paddr'])
                    max_addr = max(max_addr, segment['p_paddr'] + segment['p_memsz'])
            
            if not loadable_segments:
                print("Error: No loadable segments found in ELF file")
                return False
            
            # Create binary data buffer
            binary_size = max_addr - min_addr
            binary_data = bytearray(binary_size)
            
            # Fill segment data
            for segment in loadable_segments:
                start_offset = segment['p_paddr'] - min_addr
                segment_data = segment.data()
                binary_data[start_offset:start_offset + len(segment_data)] = segment_data
                
                print(f"Loaded segment: addr 0x{segment['p_paddr']:08X}, size {segment['p_filesz']} bytes")
            
            # Write binary file
            with open(bin_path, 'wb') as bin_file:
                bin_file.write(binary_data)
            
            file_size = len(binary_data)
            print(f'Binary generated: {os.path.basename(bin_path)} ({file_size} bytes)')
            print(f'Address range: 0x{min_addr:08X} - 0x{max_addr:08X}')
            return True
            
    except Exception as e:
        print(f'ELF to binary conversion failed: {e}')
        return False


def merge_files(bootloader_path: str, bin_path: str, target_path: str, bin_offset: int) -> bool:
    """Merge bootloader and binary files"""
    try:
        with open(target_path, 'wb') as f_target:
            # Write bootloader
            with open(bootloader_path, 'rb') as f_boot:
                boot_data = f_boot.read()
                f_target.write(boot_data)
            
            # Fill gap with 0xFF
            fill_data = b'\xFF' * (bin_offset - f_target.tell())
            f_target.write(fill_data)
            
            # Write application
            with open(bin_path, 'rb') as f_bin:
                bin_data = f_bin.read()
                f_target.write(bin_data)
        
        total_size = os.stat(target_path).st_size
        print(f'Merge completed: {os.path.basename(target_path)} ({total_size} bytes)')
        return True
    except Exception as e:
        print(f'Merge failed: {e}')
        return False

def main():
    """Main function: Process firmware files after build"""
    build_config = BuildConfig(CONFIG_FILE)
    
    try:
        # Parse command line arguments
        if len(sys.argv) < 6:
            print("Usage: post-build.py <CONFIG> <TARGET_DIR> <TARGET_NAME> <PROJ_DIR> <PROJ_NAME>")
            sys.exit(1)
            
        CONFIG_NAME = sys.argv[1]   # Build configuration
        TARGET_DIR = sys.argv[2]    # Target directory
        TARGET_BNAME = sys.argv[3]  # Target base name
        PROJ_DIR = sys.argv[4]      # Project directory
        PROJ_FNAME = sys.argv[5]    # Project file name
        OUTPUT_DIR = os.path.join(PROJ_DIR, '..', 'Output')

        print(f"Build config: {CONFIG_NAME}")
        print(f"Project dir: {PROJ_DIR}")
        print(f"Output dir: {OUTPUT_DIR}")

        # Extract version information
        VERSION_FILE_PATH = os.path.join(PROJ_DIR, '..', 'Version.h')
        project, version = extract_project_and_version(VERSION_FILE_PATH)
        if not project or not version:
            print("Failed to get project name or version")
            sys.exit(1)
            
        print(f"Project: {project} v{version}")
            
        # Generate file paths
        elf_file_path = os.path.join(TARGET_DIR, f'{TARGET_BNAME}.out')
    
        if CONFIG_NAME == 'Bootloader':
            file_suffix = '-Boot'
        elif CONFIG_NAME == 'Debug':
            file_suffix = f'-{version}-Alpha'
        else:
            file_suffix = f'-{version}'
        
        bin_file_path = os.path.join(OUTPUT_DIR, CONFIG_NAME, f'{project}{file_suffix}.bin')
        out_file_path = os.path.join(OUTPUT_DIR, CONFIG_NAME, f'{project}{file_suffix}.out')

        # Verify input file exists
        if not os.path.exists(elf_file_path):
            print(f"Error: ELF file not found: {elf_file_path}")
            sys.exit(1)

        # Check if processing is needed
        needs_processing = (
            has_git_changes_in_directory(os.path.join(PROJ_DIR, '..')) or 
            not os.path.exists(out_file_path) or 
            not os.path.exists(bin_file_path)
        )

        if needs_processing:
            print("Starting firmware processing...")
            clear_directory(os.path.join(OUTPUT_DIR, CONFIG_NAME))
            
            if not copy_file(elf_file_path, out_file_path):
                sys.exit(1)
                
            # Convert ELF to binary using pyelftools
            if not elf_to_binary(out_file_path, bin_file_path):
                sys.exit(1)

            if CONFIG_NAME == "Bootloader":
                # Update bootloader path in config
                bootloader_rel_path = os.path.join(CONFIG_NAME, f'{project}{file_suffix}.bin').replace('\\', '/')
                build_config.set_bootloader_path(bootloader_rel_path)
                print(f"Bootloader path updated: {bootloader_rel_path}")
            else:
                # Merge bootloader with application
                bootloader_rel_path = build_config.get_bootloader_path()
                bootloader_full_path = os.path.join(OUTPUT_DIR, bootloader_rel_path)
                target_file_path = os.path.join(OUTPUT_DIR, CONFIG_NAME, f'{project}{file_suffix}+boot.bin')
                
                if os.path.exists(bootloader_full_path):
                    if not merge_files(bootloader_full_path, bin_file_path, target_file_path, BIN_OFFSET):
                        sys.exit(1)
                    print(f"Bootloader merged: {os.path.basename(target_file_path)}")
                else:
                    print(f"Warning: Bootloader not found: {bootloader_full_path}")
                    print("Please build Bootloader configuration first")
        else:
            print("No processing needed (no changes detected)")
    
        # Update debug configuration
        jdebug_file = os.path.join(PROJ_DIR, f'{PROJ_FNAME}.jdebug')
        if os.path.exists(jdebug_file):
            try:
                with open(jdebug_file, "r+", encoding='utf-8') as f:
                    lines = f.readlines()
                    for i, line in enumerate(lines):
                        if 'File.Open' in line:
                            lines[i] = f'  File.Open ("$(ProjectDir)/../Output/{CONFIG_NAME}/{project}{file_suffix}.out");\n'
                            break
                    f.seek(0)
                    f.writelines(lines)
                    f.truncate()
                print(f"Debug config updated: {jdebug_file}")
            except Exception as e:
                print(f"Failed to update debug config: {e}")
        else:
            print(f"Warning: Debug config not found: {jdebug_file}")
        
        print("Firmware processing completed!")
        
    except Exception as e:
        print(f"Processing error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == '__main__':
    main()