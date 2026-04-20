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

import os
import sys
import config_util as util

os_folder = os.path.dirname(__file__) + '/..'
cfg_file = os_folder + '/.config'
output_folder = os_folder + '/../build/output/bin'

LOADABLE_BIN_TYPES = (
    ("CONFIG_SUPPORT_COMMON_BINARY=y", "COMMON"),
    ("CONFIG_APP1_INFO=y", "APP1"),
    ("CONFIG_APP2_INFO=y", "APP2"),
)


def get_flash_alignment():
    align_keys = [
        "CONFIG_AMEBASMART_FLASH_BLOCK_SIZE=",
        "CONFIG_AMEBAD_FLASH_BLOCK_SIZE=",
        "CONFIG_AMEBALITE_FLASH_BLOCK_SIZE=",
        "CONFIG_BK_FLASH_SECTOR_SIZE=",
    ]

    for key in align_keys:
        value = util.get_value_from_file(cfg_file, key).rstrip('\n')
        if value != 'None':
            return int(value, 0)

    return 4096


def get_bundle_inputs():
    image_names = []

    for config_key, bin_type in LOADABLE_BIN_TYPES:
        if util.check_config_existence(cfg_file, config_key):
            image_name = util.get_binname_from_bininfo(bin_type)
            image_path = output_folder + '/' + image_name
            if os.path.isfile(image_path):
                image_names.append(image_name)

    return image_names


def append_with_alignment(outfile, image_path, is_last, alignment):
    with open(image_path, 'rb') as image:
        outfile.write(image.read())

    if is_last:
        return

    padding_size = (-outfile.tell()) % alignment
    if padding_size:
        outfile.write(b'\xff' * padding_size)


def main():
    if not util.check_config_existence(cfg_file, 'CONFIG_APP_BINARY_SEPARATION=y'):
        return 0

    if not util.check_config_existence(cfg_file, 'CONFIG_XIP_ELF=y'):
        return 0

    user_bin_name = util.get_binname_from_bininfo("USER")
    if user_bin_name == 'None':
        return 0

    image_names = get_bundle_inputs()
    if len(image_names) == 0:
        print("No loadable images available for XIP user bundle")
        return 1

    alignment = get_flash_alignment()
    bundle_path = output_folder + '/' + user_bin_name

    with open(bundle_path, 'wb') as bundle_file:
        for index, image_name in enumerate(image_names):
            append_with_alignment(
                bundle_file,
                output_folder + '/' + image_name,
                index == len(image_names) - 1,
                alignment)

    print("CP: " + user_bin_name)
    return 0


if __name__ == "__main__":
    sys.exit(main())
