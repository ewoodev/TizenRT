#!/usr/bin/env python
###########################################################################
#
# Copyright 2024 Samsung Electronics All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
# either express or implied. See the License for the specific
# language governing permissions and limitations under the License.
#
###########################################################################

import os
import sys
import string
import subprocess
import tempfile
import config_util as util

os_folder = os.path.dirname(__file__) + '/..'
cfg_file = os_folder + '/.config'
tool_folder = os_folder + '/tools'
build_folder = os_folder + '/../build'
output_folder = build_folder + '/output/bin/'

# Create output directory if it doesnt exist
if not os.path.exists(output_folder):
    os.makedirs(output_folder)

CONFIG_ARCH_BOARD = util.get_value_from_file(cfg_file, "CONFIG_ARCH_BOARD=").rstrip('\n')
board_name = CONFIG_ARCH_BOARD[1:-1]
CONFIG_TRPK_CONTAINS_MULTIPLE_BINARY = util.get_value_from_file(cfg_file, "CONFIG_TRPK_CONTAINS_MULTIPLE_BINARY=").rstrip('\n')
# Get flash virtual remapped address instead of physical address
CONFIG_FLASH_VSTART_LOADABLE = util.get_value_from_file(cfg_file, "CONFIG_FLASH_VSTART_LOADABLE=").rstrip('\n')

# Define boards that use dual mode (dual OTA support)
DUAL_LD_BOARDS = ["rtl8730e"]
is_dual_ld_mode = board_name in DUAL_LD_BOARDS

# Dynamically get the offset from Kernel TRPK binary file
# Chip specific should implement the logic for offset calculation according to trpk file content
offset = 0
if CONFIG_TRPK_CONTAINS_MULTIPLE_BINARY == "y":
    if is_dual_ld_mode:
        sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../../build/tools/amebasmart/gnu_utility')))
        from loadable_xip_elf import get_offset
        offset_shift = get_offset()
        offset = int(CONFIG_FLASH_VSTART_LOADABLE, 16) - int(offset_shift, 16)
    else:
        if util.check_config_existence(cfg_file, 'CONFIG_ARCH_CHIP_ARMINO=y'):
            # Armino (e.g. bk7239n): use chip-specific script to compute flash start from TRPK
            _vstart_script = os.path.abspath(os.path.join(os.path.dirname(__file__), '../../build/configs/bk7239n/get_flash_vstart_loadable.py'))
            computed = subprocess.check_output([sys.executable, _vstart_script, os.path.abspath(cfg_file)]).decode('utf-8').strip()
            if computed:
                CONFIG_FLASH_VSTART_LOADABLE = computed
            else:
                CONFIG_FLASH_VSTART_LOADABLE = util.get_value_from_file(cfg_file, "CONFIG_FLASH_VSTART_LOADABLE=").rstrip('\n')
        else:
            CONFIG_FLASH_VSTART_LOADABLE = util.get_value_from_file(cfg_file, "CONFIG_FLASH_VSTART_LOADABLE=").rstrip('\n')
        offset = int(CONFIG_FLASH_VSTART_LOADABLE, 16)
else:
    offset = int(CONFIG_FLASH_VSTART_LOADABLE, 16)

PART_IDX = 0

PARTITION_SIZE_LIST = util.get_value_from_file(cfg_file, "CONFIG_FLASH_PART_SIZE=").rstrip('\n').strip('"').rstrip(',')
PARTITION_NAME_LIST = util.get_value_from_file(cfg_file, "CONFIG_FLASH_PART_NAME=").rstrip('\n').strip('"').rstrip(',')

CONFIG_APP_BINARY_SEPARATION = util.get_value_from_file(cfg_file, "CONFIG_APP_BINARY_SEPARATION=").rstrip('\n')
CONFIG_SUPPORT_COMMON_BINARY = util.get_value_from_file(cfg_file, "CONFIG_SUPPORT_COMMON_BINARY=").rstrip('\n')
CONFIG_USE_BP = util.get_value_from_file(cfg_file, "CONFIG_USE_BP=").rstrip('\n')
CONFIG_NUM_APPS = util.get_value_from_file(cfg_file, "CONFIG_NUM_APPS=").rstrip('\n')

if PARTITION_SIZE_LIST == 'None' :
    sys.exit(0)

NAME_LIST = PARTITION_NAME_LIST.split(",")
SIZE_LIST = PARTITION_SIZE_LIST.split(",")

# Initialize data structures based on mode
if is_dual_ld_mode:
    # dual mode: use 2D array to store scripts for dual OTA partitions
    # ld_scripts[0] = common scripts for [slot0, slot1]
    # ld_scripts[1] = app1 scripts for [slot0, slot1]
    # ld_scripts[2] = app2 scripts for [slot0, slot1]
    ld_scripts = [[], [], []]  # three lists: [common, app1, app2] for two OTA partitions
    for i in range(2):  # Two OTA slots: 0 and 1
        start = "/* Auto-generated ld script */\nMEMORY\n"
        start += "{\n   uflash (rx)      : ORIGIN = "
        ld_scripts[0].append(start)  # common slot0 and slot1
        ld_scripts[1].append(start)  # app1 slot0 and slot1
        if int(CONFIG_NUM_APPS) >= 2:
            ld_scripts[2].append(start)  # app2 slot0 and slot1
    # Track which OTA slots have been generated for each binary type
    ld_generated_rtl = {"common": [False, False], "app1": [False, False], "app2": [False, False]}
    first_loadable_encountered = False
else:
    # Generic mode: track which ld files have been generated
    ld_generated = {"common": False, "app1": False, "app2": False}
    first_loadable_encountered = False
    ld_scripts = None
    ld_generated_rtl = None

ota_index = 1

CONFIG_RAM_SIZE = util.get_value_from_file(cfg_file, "CONFIG_RAM_SIZE=").rstrip('\n')
CONFIG_RAM_START = util.get_value_from_file(cfg_file, "CONFIG_RAM_START=").rstrip('\n')

str1 = " , LENGTH = "
str2 = "\n   usram (rwx)      : ORIGIN = "

ram_end = int(CONFIG_RAM_SIZE) + int(CONFIG_RAM_START, 16)

if ram_end % 4096 != 0 :
    print("RAM end should be 4KB aligned")
    sys.exit(1)

ram_offset = ram_end
ram_size = 0

CONFIG_APP1_BIN_DYN_RAMSIZE=util.get_value_from_file(cfg_file, "CONFIG_APP1_BIN_DYN_RAMSIZE=").rstrip('\n')
if util.check_config_existence(cfg_file, 'CONFIG_APP2_INFO') == True :
    CONFIG_APP2_BIN_DYN_RAMSIZE=util.get_value_from_file(cfg_file, "CONFIG_APP2_BIN_DYN_RAMSIZE=").rstrip('\n')
CONFIG_COMMON_BIN_STATIC_RAMSIZE=util.get_value_from_file(cfg_file, "CONFIG_COMMON_BIN_STATIC_RAMSIZE=").rstrip('\n')

ram_offset = ram_offset - int(CONFIG_COMMON_BIN_STATIC_RAMSIZE)
if ram_offset % 4096 != 0 :
    print("!!!!!!!!!!!!!!!!!!!!!! ERROR !!!!!!!!!!!!!!!!!!!!!!!!!!")
    print("CONFIG_COMMON_BIN_STATIC_RAMSIZE should be aligned to 4KB")
    sys.exit(1)

common_ram_size = (int(CONFIG_COMMON_BIN_STATIC_RAMSIZE) - 64 * 1024) #remove the pg table size at the end, will have to generalize it later
common_ram_str = hex(ram_offset) + str1 + hex(common_ram_size) + "\n}\n"

ram_offset = ram_offset - int(CONFIG_APP1_BIN_DYN_RAMSIZE)
if ram_offset % 4096 != 0 :
    print("!!!!!!!!!!!!!!!!!!!!!! ERROR !!!!!!!!!!!!!!!!!!!!!!!!!!")
    print("CONFIG_APP1_BIN_DYN_RAMSIZE should be aligned to 4KB")
    sys.exit(1)

app1_ram_str = hex(ram_offset) + str1 + hex(int(CONFIG_APP1_BIN_DYN_RAMSIZE)) + "\n}\n"

# Initialize app2_ram_str to app1_ram_str as default
app2_ram_str = app1_ram_str
if util.check_config_existence(cfg_file, 'CONFIG_APP2_INFO') == True :
    ram_offset = ram_offset - int(CONFIG_APP2_BIN_DYN_RAMSIZE)
    app2_ram_str = hex(ram_offset) + str1 + hex(int(CONFIG_APP2_BIN_DYN_RAMSIZE)) + "\n}\n"
    if ram_offset % 4096 != 0 :
        print("!!!!!!!!!!!!!!!!!!!!!! ERROR !!!!!!!!!!!!!!!!!!!!!!!!!!")
        print("CONFIG_APP2_BIN_DYN_RAMSIZE should be aligned to 4KB")
        sys.exit(1)

# RAM string mapping for apps
app_ram_str = {"app1": app1_ram_str, "app2": app2_ram_str}

CONFIG_USER_SIGN_PREPEND_SIZE = util.get_value_from_file(cfg_file, "CONFIG_USER_SIGN_PREPEND_SIZE=").rstrip('\n')

if CONFIG_USER_SIGN_PREPEND_SIZE == 'None' :
    signing_offset = 0
else :
    signing_offset = int(CONFIG_USER_SIGN_PREPEND_SIZE)

LOADABLE_TARGETS = ("common", "app1", "app2")
XIP_OBJCOPY = "arm-none-eabi-objcopy"

def align_up(value, alignment):
    return ((value + alignment - 1) // alignment) * alignment

def get_flash_alignment():
    align_config_names = [
        "CONFIG_AMEBASMART_FLASH_BLOCK_SIZE=",
        "CONFIG_AMEBAD_FLASH_BLOCK_SIZE=",
        "CONFIG_AMEBALITE_FLASH_BLOCK_SIZE=",
        "CONFIG_BK_FLASH_SECTOR_SIZE=",
    ]

    for config_name in align_config_names:
        align_value = util.get_value_from_file(cfg_file, config_name).rstrip('\n')
        if align_value != 'None':
            return int(align_value, 0)

    return 4096

def get_loadable_order():
    loadable_order = []

    if CONFIG_SUPPORT_COMMON_BINARY == 'y':
        loadable_order.append("common")
    if util.check_config_existence(cfg_file, 'CONFIG_APP1_INFO') == True:
        loadable_order.append("app1")
    if util.check_config_existence(cfg_file, 'CONFIG_APP2_INFO') == True:
        loadable_order.append("app2")

    return loadable_order

def get_loadable_output_name(target_name):
    loadable_name_map = {
        "common": "CONFIG_COMMON_BINARY_NAME=",
        "app1": "CONFIG_APP1_BIN_NAME=",
        "app2": "CONFIG_APP2_BIN_NAME=",
    }

    return util.get_value_from_file(cfg_file, loadable_name_map[target_name]).replace('"', '').rstrip('\n')

def get_payload_offset(target_name):
    if target_name == "common":
        return signing_offset + 0x10
    return signing_offset + 0x30

def get_payload_offset_without_sign(target_name):
    if target_name == "common":
        return 0x10
    return 0x30

def get_target_ram_str(target_name):
    if target_name == "common":
        return common_ram_str
    return app_ram_str[target_name]

def get_loadable_slot_index(target_name):
    for slot_index, name in enumerate(NAME_LIST):
        if name == target_name:
            return slot_index
    raise RuntimeError("No %s partition configured for XIP loadable build" % target_name)

def get_loadable_slot_size():
    loadable_order = get_loadable_order()
    if len(loadable_order) == 0:
        raise RuntimeError("No loadable binaries configured")

    start_index = get_loadable_slot_index(loadable_order[0])
    slot_size = 0

    for slot_name, slot_size_kb in zip(NAME_LIST[start_index:], SIZE_LIST[start_index:]):
        if slot_name not in LOADABLE_TARGETS:
            break
        slot_size += int(slot_size_kb) * 1024

    return slot_size

def get_binary_file_size(binary_path):
    temp_fd, temp_bin_path = tempfile.mkstemp(prefix="mkldscript_", suffix=".bin")
    os.close(temp_fd)

    try:
        subprocess.check_call([XIP_OBJCOPY, "-O", "binary", binary_path, temp_bin_path])
        return os.path.getsize(temp_bin_path)
    finally:
        if os.path.exists(temp_bin_path):
            os.remove(temp_bin_path)

def estimate_packaged_span(target_name):
    binary_name = get_loadable_output_name(target_name)
    binary_path = output_folder + binary_name

    if not os.path.isfile(binary_path):
        raise RuntimeError("Expected %s before generating %s_0.ld" % (binary_path, target_name))

    raw_binary_size = get_binary_file_size(binary_path)
    payload_offset = get_payload_offset_without_sign(target_name)

    if util.get_value_from_file(cfg_file, "CONFIG_ARCH_CHIP_ARMINO=").rstrip('\n') != 'None':
        padding_size = 32 - ((payload_offset + raw_binary_size) % 32)
        if padding_size > 0:
            raw_binary_size += padding_size

    total_span = signing_offset + payload_offset + raw_binary_size
    return total_span

def get_target_start(target_name):
    loadable_order = get_loadable_order()
    current_offset = offset
    flash_alignment = get_flash_alignment()

    for loadable_name in loadable_order:
        if loadable_name == target_name:
            return current_offset
        current_offset = align_up(current_offset + estimate_packaged_span(loadable_name), flash_alignment)

    raise RuntimeError("Unsupported XIP loadable target: %s" % target_name)

def generate_targeted_ld_script(target_name):
    loadable_order = get_loadable_order()
    if target_name not in loadable_order:
        raise RuntimeError("%s is not enabled in the current configuration" % target_name)

    slot_start = offset
    slot_size = get_loadable_slot_size()
    target_start = get_target_start(target_name)
    payload_offset = get_payload_offset(target_name)
    flash_size = slot_size - (target_start - slot_start) - payload_offset

    if flash_size <= 0:
        raise RuntimeError("No flash space left for %s after loadable packing" % target_name)

    ld_file = output_folder + target_name + "_0.ld"
    flash_start = hex(target_start + payload_offset)
    with open(ld_file, "w") as ld:
        print("Generating " + os.path.basename(ld_file) + " for sequential XIP loadable packing")
        ld.write(generate_ld_script(flash_start, hex(flash_size), get_target_ram_str(target_name)))

if len(sys.argv) > 1 and sys.argv[1] in LOADABLE_TARGETS:
    generate_ld_script = lambda flash_start, flash_size, ram_str: "/* Auto-generated ld script */\nMEMORY\n{\n   uflash (rx)      : ORIGIN = " + flash_start + str1 + flash_size + str2 + ram_str
    try:
        generate_targeted_ld_script(sys.argv[1])
    except RuntimeError as error:
        print("Error: " + str(error))
        sys.exit(1)
    sys.exit(0)

# Helper function to reset offset for generic mode boards (e.g., bk7239n)
def reset_offset_if_needed():
    global offset, first_loadable_encountered
    if not is_dual_ld_mode and board_name == "bk7239n" and not first_loadable_encountered:
        offset = int(CONFIG_FLASH_VSTART_LOADABLE, 16)
        first_loadable_encountered = True

# Helper function to generate ld script content
def generate_ld_script(flash_start, flash_size, ram_str):
    ld_script_content = "/* Auto-generated ld script */\nMEMORY\n"
    ld_script_content += "{\n   uflash (rx)      : ORIGIN = "
    ld_script_content += flash_start + str1 + flash_size + str2 + ram_str
    return ld_script_content

for name in NAME_LIST :
    part_size = int(SIZE_LIST[PART_IDX]) * 1024

    # Reset offset for first loadable partition on generic mode boards (e.g., bk7239n)
    if not is_dual_ld_mode and name in ("app1", "app2", "common") and not first_loadable_encountered:
        reset_offset_if_needed()

    if name == "kernel" :
        ota_index = (ota_index + 1) % 2
    elif name == "app1" :
        if is_dual_ld_mode:
            # dual mode: generate dual OTA files
            # Use current ota_index to determine which slot this partition belongs to
            # Generate file for current slot if not already generated
            # Note: ota_index is switched when encountering "kernel", not here
            if not ld_generated_rtl["app1"][ota_index]:
                app1_start = hex(offset + 0x30 + signing_offset)
                app1_size = hex(part_size - 0x30 - signing_offset)
                ld_scripts[1][ota_index] = ld_scripts[1][ota_index] + app1_start + str1 + app1_size + str2 + app1_ram_str
                with open(output_folder + "app1_" + str(ota_index) + ".ld", "w") as ld :
                    ld.write(ld_scripts[1][ota_index])
                ld_generated_rtl["app1"][ota_index] = True
        else:
            # Generic mode: generate single OTA file (_0.ld only)
            ota_index = (ota_index + 1) % 2
            if not ld_generated["app1"]:
                app1_start = hex(offset + 0x30 + signing_offset)
                app1_size = hex(part_size - 0x30 - signing_offset)
                ld_file = "app1_0.ld"
                with open(output_folder + ld_file, "w") as ld :
                    print("Generating " + ld_file + " for position-independent code")
                    ld.write(generate_ld_script(app1_start, app1_size, app_ram_str["app1"]))
                ld_generated["app1"] = True
    elif name == "app2" :
        if is_dual_ld_mode:
            # dual mode: generate dual OTA files
            # Use current ota_index to determine which slot this partition belongs to
            # Generate file for current slot if not already generated
            if not ld_generated_rtl["app2"][ota_index]:
                app2_start = hex(offset + 0x30 + signing_offset)
                app2_size = hex(part_size - 0x30 - signing_offset)
                ld_scripts[2][ota_index] = ld_scripts[2][ota_index] + app2_start + str1 + app2_size + str2 + app2_ram_str
                with open(output_folder + "app2_" + str(ota_index) + ".ld", "w") as ld :
                    ld.write(ld_scripts[2][ota_index])
                ld_generated_rtl["app2"][ota_index] = True
            # Note: ota_index is switched when encountering "kernel", not here
        else:
            # Generic mode: generate single OTA file (_0.ld only)
            if not ld_generated["app2"]:
                app2_start = hex(offset + 0x30 + signing_offset)
                app2_size = hex(part_size - 0x30 - signing_offset)
                ld_file = "app2_0.ld"
                with open(output_folder + ld_file, "w") as ld :
                    print("Generating " + ld_file + " for position-independent code")
                    ld.write(generate_ld_script(app2_start, app2_size, app_ram_str["app2"]))
                ld_generated["app2"] = True
    elif name == "common" :
        if is_dual_ld_mode:
            # dual mode: generate dual OTA files
            # Use current ota_index to determine which slot this partition belongs to
            # Generate file for current slot if not already generated
            if not ld_generated_rtl["common"][ota_index]:
                common_start = hex(offset + 0x10 + signing_offset)
                common_size = hex(part_size - 0x10 - signing_offset)
                ld_scripts[0][ota_index] = ld_scripts[0][ota_index] + common_start + str1 + common_size + str2 + common_ram_str
                with open(output_folder + "common_" + str(ota_index) + ".ld", "w") as ld :
                    ld.write(ld_scripts[0][ota_index])
                ld_generated_rtl["common"][ota_index] = True
            # Note: ota_index is switched when encountering "kernel", not here
        else:
            # Generic mode: generate single OTA file (_0.ld only)
            if not ld_generated["common"] and CONFIG_SUPPORT_COMMON_BINARY == 'y':
                common_start = hex(offset + 0x10 + signing_offset)
                common_size = hex(part_size - 0x10 - signing_offset)
                ld_file = "common_0.ld"
                with open(output_folder + ld_file, "w") as ld :
                    print("Generating " + ld_file + " for position-independent code")
                    ld.write(generate_ld_script(common_start, common_size, common_ram_str))
                ld_generated["common"] = True
    else:
        PART_IDX = PART_IDX + 1
        continue

    if name == 'app1' or name == 'app2' or name == 'common' :
        if offset % 4096 != 0 :
            print("!!!!!!!!!!!!!!!!!!!!!! ERROR !!!!!!!!!!!!!!!!!!!!!!!!!!")
            print("flash start [" + hex(offset) + "] of " + name + " should be aligned to 4KB")
            sys.exit(1)
        if part_size % 4096 != 0 :
            print("!!!!!!!!!!!!!!!!!!!!!! ERROR !!!!!!!!!!!!!!!!!!!!!!!!!!")
            print("flash partition size [" + str(part_size) + "] of " + name + " should be aligned to 4KB")
            sys.exit(1)

    offset = offset + part_size
    PART_IDX = PART_IDX + 1
