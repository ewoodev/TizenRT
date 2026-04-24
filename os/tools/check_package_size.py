#!/usr/bin/env python
###########################################################################
#
# Copyright 2021 Samsung Electronics All Rights Reserved.
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

# This script checks if the each partitions are large enough to accommodate their respective binaries.
# It gets the partition size and binary size and check if the binary sizes are smaller than their respective partition sizes.

import os
import sys
import config_util as util
import shared_slot_tool as shared_slot

os_folder = os.path.dirname(__file__) + '/..'
cfg_file = os_folder + '/.config'
tool_folder = os_folder + '/tools'
build_folder = os_folder + '/../build'
output_folder = build_folder + '/output/bin'
output_file_name = output_folder + '/tinyara_binarysize.txt'

INTERNAL_FLASH = 0
EXTERNAL_FLASH = 1

CONFIG_APP_BINARY_SEPARATION = util.get_value_from_file(cfg_file, "CONFIG_APP_BINARY_SEPARATION=").rstrip('\n')
CONFIG_SUPPORT_COMMON_BINARY = util.get_value_from_file(cfg_file, "CONFIG_SUPPORT_COMMON_BINARY=").rstrip('\n')
CONFIG_RESOURCE_FS = util.get_value_from_file(cfg_file, "CONFIG_RESOURCE_FS=").rstrip('\n')

INTERNAL_PARTITION_NAME_LIST = util.get_value_from_file(cfg_file, "CONFIG_FLASH_PART_NAME=")
INTERNAL_PARTITION_SIZE_LIST = util.get_value_from_file(cfg_file, "CONFIG_FLASH_PART_SIZE=")

CONFIG_SECOND_FLASH_PARTITION = util.get_value_from_file(cfg_file, "CONFIG_SECOND_FLASH_PARTITION=").rstrip('\n')
SECOND_PARTITION_NAME_LIST = util.get_value_from_file(cfg_file, "CONFIG_SECOND_FLASH_PART_NAME=")
SECOND_PARTITION_SIZE_LIST = util.get_value_from_file(cfg_file, "CONFIG_SECOND_FLASH_PART_SIZE=")

FAIL_TO_BUILD = False
WARNING_RATIO = 95
fail_type_list = []

def number_with_comma_align(number):
    return ("{:10,}".format(number))

def number_with_comma(number):
    return ("{:,}".format(number))

def write_validation_line(text):
    with open(output_file_name, 'a') as outfile:
        outfile.write(text + '\n')


def get_output_path(bin_type):
    bin_name = util.get_binname_from_bininfo(bin_type)
    if bin_name == 'None':
        return None
    return output_folder + '/' + bin_name


def get_result(partition_size, binary_size):
    used_ratio = 0
    if partition_size != 0:
        used_ratio = round(float(binary_size) / float(partition_size) * 100, 2)

    if partition_size < int(binary_size):
        return used_ratio, "FAIL", ""
    if used_ratio > WARNING_RATIO:
        return used_ratio, "WARNING", ":warning:"
    return used_ratio, "PASS", ":heavy_check_mark:"


def report_validation(bin_type, binary_size, partition_size, output_path=None):
    used_ratio, check_result, result_mark = get_result(partition_size, binary_size)

    if check_result == "FAIL":
        fail_type_list.append(bin_type)
        if output_path and os.path.isfile(output_path):
            os.remove(output_path)
        global FAIL_TO_BUILD
        FAIL_TO_BUILD = True

    print(" {:10}".format(bin_type) + " " + number_with_comma_align(binary_size) + " bytes   " +
            number_with_comma_align(partition_size) + " bytes    " + "{:6}".format(used_ratio) + "%    " + check_result)
    write_validation_line(bin_type + " | " + number_with_comma(binary_size) + " bytes | " +
        number_with_comma(partition_size) + " bytes | " + str(used_ratio) + "% | " + result_mark + check_result)


def validate_binary_size(bin_type, part_size):
    output_path = get_output_path(bin_type)
    if output_path is None:
        return

    binary_size = os.path.getsize(output_path)
    report_validation(bin_type, binary_size, part_size, output_path)

def check_part_size(flash_type, bin_type):
    if flash_type == INTERNAL_FLASH :
        PARTITION_NAME_LIST = INTERNAL_PARTITION_NAME_LIST
        PARTITION_SIZE_LIST = INTERNAL_PARTITION_SIZE_LIST
    else :
        PARTITION_NAME_LIST = SECOND_PARTITION_NAME_LIST
        PARTITION_SIZE_LIST = SECOND_PARTITION_SIZE_LIST

    if PARTITION_SIZE_LIST == 'None' :
        sys.exit(0)

    NAME_LIST = PARTITION_NAME_LIST.replace('"','').split(",")
    SIZE_LIST = PARTITION_SIZE_LIST.replace('"','').split(",")

    PART_IDX = 0

    for name in NAME_LIST :
        if (name.lower() == bin_type.lower()) or (bin_type == "KERNEL" and name.lower() == "os") :
            return int(SIZE_LIST[PART_IDX]) * 1024
        PART_IDX += 1
    return 0

def check_binary_size(bin_name):
    part_size = check_part_size(INTERNAL_FLASH, bin_name)
    if part_size == 0 and CONFIG_SECOND_FLASH_PARTITION == "y" :
        part_size = check_part_size(EXTERNAL_FLASH, bin_name)

    validate_binary_size(bin_name, part_size)


def is_shared_slot_validation_enabled():
    return shared_slot.is_nonxip_shared_slot_enabled()


def get_enabled_shared_slot_targets():
    targets = ["kernel"]

    if util.check_config_existence(cfg_file, 'CONFIG_SUPPORT_COMMON_BINARY=y') == True:
        targets.append("common")
    if util.check_config_existence(cfg_file, 'CONFIG_APP1_INFO=y') == True:
        targets.append("app1")
    if util.check_config_existence(cfg_file, 'CONFIG_APP2_INFO=y') == True:
        targets.append("app2")

    return targets


def validate_shared_slot_size():
    targets = get_enabled_shared_slot_targets()
    slot_start = shared_slot.get_partition_offset("kernel")
    slot_end = shared_slot.get_shared_slot_end()
    slot_size = slot_end - slot_start
    flash_alignment = shared_slot.get_flash_alignment()
    current_start = slot_start

    print("\nShared slot layout details:")
    for index, target_name in enumerate(targets):
        output_path = shared_slot.get_package_path(target_name)
        binary_size = os.path.getsize(output_path)
        remaining_size = slot_end - current_start
        bin_type = shared_slot.get_bininfo_key(target_name)

        report_validation(bin_type, binary_size, remaining_size, output_path)

        current_end = current_start + binary_size
        padding_size = 0
        if index != len(targets) - 1:
            aligned_end = shared_slot.align_up(current_end, flash_alignment)
            padding_size = aligned_end - current_end
            current_start = aligned_end
        else:
            current_start = current_end

        print("  {:8} start={} end={} pad={}".format(
            target_name.upper(),
            hex(current_end - binary_size),
            hex(current_end),
            padding_size))

    used_total = current_start - slot_start
    report_validation("SHARED_SLOT", used_total, slot_size)

# Check if the binary size is smaller than its partition size
print("\n========== Size Verification of built Binaries ==========")
print("Type        Binary Size     Partition Size      used(%)")

# File print init
outfile = open(output_file_name, 'w')
outfile.write("========== Size Verification of built Binaries ==========\n")
outfile.write("Type | Binary Size | Partition Size | used(%) | result\n")
outfile.write("-- | -- | -- | -- | --\n")
outfile.close()

if is_shared_slot_validation_enabled():
    validate_shared_slot_size()
else:
    check_binary_size("KERNEL")
    if CONFIG_APP_BINARY_SEPARATION == "y" :
        check_binary_size("APP1")
        check_binary_size("APP2")
        if CONFIG_SUPPORT_COMMON_BINARY == "y" :
            check_binary_size("COMMON")
if CONFIG_RESOURCE_FS == "y" :
    check_binary_size("RESOURCE")

if FAIL_TO_BUILD == True :
    # Stop to build, because there is mismatched size problem.
    print("!!!!!!!! ERROR !!!!!!!")
    for fail_type in fail_type_list :
        print("=> " + fail_type + " Binary will be deleted. Need to re-configure the partition using menuconfig and to re-build.")
    sys.exit(1)
else :
    print("=> Size verification SUCCESS!! The size of all binaries are OK.\n")
