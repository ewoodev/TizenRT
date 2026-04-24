#!/usr/bin/env python
###########################################################################
#
# Copyright 2026 Samsung Electronics All Rights Reserved.
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

from __future__ import print_function

import os
import struct
import sys

import config_util as util

OS_FOLDER = os.path.dirname(__file__) + "/.."
CFG_FILE = OS_FOLDER + "/.config"
OUTPUT_FOLDER = OS_FOLDER + "/../build/output/bin"

CHECKSUM_SIZE = 4
SHARED_SLOT_TARGETS = ("kernel", "common", "app1", "app2")


def parse_int(value, default=None):
    if value == "None":
        return default
    return int(str(value).strip().strip('"'), 0)


def align_up(value, alignment):
    return ((value + alignment - 1) // alignment) * alignment


def get_flash_alignment():
    align_keys = [
        "CONFIG_AMEBASMART_FLASH_BLOCK_SIZE=",
        "CONFIG_AMEBAD_FLASH_BLOCK_SIZE=",
        "CONFIG_AMEBALITE_FLASH_BLOCK_SIZE=",
        "CONFIG_BK_FLASH_SECTOR_SIZE=",
    ]

    for key in align_keys:
        value = util.get_value_from_file(CFG_FILE, key).rstrip("\n")
        if value != "None":
            return int(value, 0)

    return 4096


def get_signing_size():
    value = util.get_value_from_file(CFG_FILE, "CONFIG_USER_SIGN_PREPEND_SIZE=").rstrip("\n")
    if value == "None":
        return 0
    return int(value, 0)


def is_xip_enabled():
    return util.check_config_existence(CFG_FILE, "CONFIG_XIP_ELF=y")


def is_nonxip_shared_slot_enabled():
    return util.check_config_existence(CFG_FILE, "CONFIG_APP_BINARY_SEPARATION=y") and \
        util.check_config_existence(CFG_FILE, "CONFIG_SUPPORT_COMMON_BINARY=y") and \
        not is_xip_enabled()


def get_expected_header_size(role):
    if role == "app":
        return 44 if is_xip_enabled() else 41
    if role == "common":
        return 12 if is_xip_enabled() else 10
    if role == "resource":
        return 10
    raise RuntimeError("Unsupported package role: %s" % role)


def read_u16(data, offset):
    return struct.unpack_from("<H", data, offset)[0]


def read_u32(data, offset):
    return struct.unpack_from("<I", data, offset)[0]


def get_signing_offset(data, expected_header_size):
    unsigned_header_size = read_u16(data, CHECKSUM_SIZE)
    if unsigned_header_size == expected_header_size:
        return 0

    signing_size = get_signing_size()
    if signing_size > 0 and len(data) >= CHECKSUM_SIZE + signing_size + 2:
        signed_header_size = read_u16(data, CHECKSUM_SIZE + signing_size)
        if signed_header_size == expected_header_size:
            return signing_size

    raise RuntimeError("Unable to detect the package header position")


def get_package_role(target_name):
    if target_name in ("app1", "app2"):
        return "app"
    return target_name


def get_bininfo_key(target_name):
    key_map = {
        "kernel": "KERNEL",
        "common": "COMMON",
        "app1": "APP1",
        "app2": "APP2",
        "resource": "RESOURCE",
    }

    if target_name not in key_map:
        raise RuntimeError("Unsupported target: %s" % target_name)
    return key_map[target_name]


def get_package_path(target_name):
    package_name = util.get_binname_from_bininfo(get_bininfo_key(target_name))
    if package_name == "None":
        raise RuntimeError("No package name found for %s" % target_name)

    package_path = OUTPUT_FOLDER + "/" + package_name
    if not os.path.isfile(package_path):
        raise RuntimeError("Required package is missing: %s" % package_path)

    return package_path


def get_package_span(target_name):
    package_path = get_package_path(target_name)
    package_role = get_package_role(target_name)

    with open(package_path, "rb") as package_file:
        data = package_file.read()

    if package_role == "kernel":
        signing_offset = 0
        header_offset = CHECKSUM_SIZE
        header_size = read_u16(data, header_offset)
        file_size = read_u32(data, header_offset + 6)
    elif package_role == "app":
        signing_offset = get_signing_offset(data, get_expected_header_size(package_role))
        header_offset = CHECKSUM_SIZE + signing_offset
        header_size = read_u16(data, header_offset)
        file_size = read_u32(data, header_offset + 5)
    else:
        signing_offset = get_signing_offset(data, get_expected_header_size(package_role))
        header_offset = CHECKSUM_SIZE + signing_offset
        header_size = read_u16(data, header_offset)
        file_size = read_u32(data, header_offset + 6)

    total_span = CHECKSUM_SIZE + signing_offset + header_size + file_size
    if total_span > os.path.getsize(package_path):
        raise RuntimeError("Package header exceeds the actual file size: %s" % package_path)

    return total_span


def get_partition_layout():
    partition_names = util.get_value_from_file(CFG_FILE, "CONFIG_FLASH_PART_NAME=").rstrip("\n")
    partition_sizes = util.get_value_from_file(CFG_FILE, "CONFIG_FLASH_PART_SIZE=").rstrip("\n")
    flash_start = util.get_value_from_file(CFG_FILE, "CONFIG_FLASH_START_ADDR=").rstrip("\n")

    if partition_names == "None" or partition_sizes == "None" or flash_start == "None":
        raise RuntimeError("Flash partition configuration is incomplete")

    names = [name.strip() for name in partition_names.strip('"').split(",") if name.strip()]
    sizes = [int(size.strip()) * 1024 for size in partition_sizes.strip('"').split(",") if size.strip()]
    if len(names) != len(sizes):
        raise RuntimeError("Partition name and size counts are mismatched")

    return parse_int(flash_start), names, sizes


def get_partition_offset(target_name):
    current_offset, names, sizes = get_partition_layout()

    for name, size in zip(names, sizes):
        if name == target_name:
            return current_offset
        current_offset += size

    raise RuntimeError("Partition %s is not configured" % target_name)


def get_shared_slot_end():
    current_offset, names, sizes = get_partition_layout()
    found_slot = False

    for name, size in zip(names, sizes):
        if not found_slot:
            if name == "kernel":
                found_slot = True
                current_offset += size
            else:
                current_offset += size
            continue

        if name not in SHARED_SLOT_TARGETS:
            return current_offset
        current_offset += size

    if found_slot:
        return current_offset

    raise RuntimeError("No shared slot was found in the partition table")


def get_nonxip_target_layout(target_name):
    if not is_nonxip_shared_slot_enabled():
        raise RuntimeError("Non-XIP shared slot is not enabled")

    if target_name not in SHARED_SLOT_TARGETS:
        raise RuntimeError("Unsupported shared-slot target: %s" % target_name)

    alignment = get_flash_alignment()
    shared_slot_start = get_partition_offset("kernel")
    shared_slot_end = get_shared_slot_end()

    current_start = shared_slot_start
    if target_name == "kernel":
        return current_start, shared_slot_end - current_start

    current_start = align_up(current_start + get_package_span("kernel"), alignment)
    if target_name == "common":
        return current_start, shared_slot_end - current_start

    current_start = align_up(current_start + get_package_span("common"), alignment)
    if target_name == "app1":
        return current_start, shared_slot_end - current_start

    current_start = align_up(current_start + get_package_span("app1"), alignment)
    if target_name == "app2":
        return current_start, shared_slot_end - current_start

    raise RuntimeError("Unhandled target: %s" % target_name)


def print_download_info(target_name):
    start_addr, remaining_size = get_nonxip_target_layout(target_name)
    size_kb = (remaining_size + 1023) // 1024
    print("%s %d" % (hex(start_addr), size_kb))


def main():
    if len(sys.argv) != 3:
        print("Usage: %s download-info <target>" % sys.argv[0], file=sys.stderr)
        return 1

    command = sys.argv[1]
    target_name = sys.argv[2].lower()

    try:
        if command == "download-info":
            print_download_info(target_name)
            return 0
        raise RuntimeError("Unsupported command: %s" % command)
    except RuntimeError as error:
        print("Error: %s" % error, file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
