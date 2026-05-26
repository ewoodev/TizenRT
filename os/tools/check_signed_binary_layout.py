#!/usr/bin/env python3
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
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
###########################################################################

import argparse
import os
import struct
import sys

CHECKSUM_SIZE = 4
RESOURCE_HEADER_SIZE = 4096


def read_config_value(config_path, key, default=None):
    if not config_path or not os.path.isfile(config_path):
        return default

    prefix = key + "="
    with open(config_path, "r", encoding="utf-8", errors="ignore") as fp:
        for line in fp:
            line = line.strip()
            if line.startswith(prefix):
                return line[len(prefix):].strip().strip('"')
    return default


def get_sign_size(args):
    if args.sign_size is not None:
        return args.sign_size

    value = read_config_value(args.config, "CONFIG_USER_SIGN_PREPEND_SIZE", "0")
    try:
        return int(value, 0)
    except ValueError:
        print("Invalid CONFIG_USER_SIGN_PREPEND_SIZE: %s" % value)
        return None


def read_file(path):
    with open(path, "rb") as fp:
        return fp.read()


def unpack_u16(data, offset):
    return struct.unpack_from("<H", data, offset)[0]


def unpack_u32(data, offset):
    return struct.unpack_from("<I", data, offset)[0]


def fail(message):
    print("ERROR: %s" % message)
    return 1


def check_user_common(data, binary_type, sign_size):
    if len(data) < CHECKSUM_SIZE + 2:
        return fail("binary is too small to contain checksum and header")

    header_size = unpack_u16(data, CHECKSUM_SIZE)
    header_end = CHECKSUM_SIZE + header_size
    sign_end = header_end + sign_size

    if header_end > len(data):
        return fail("header exceeds file size")
    if sign_end > len(data):
        return fail("signing area exceeds file size")

    bin_size_offset = CHECKSUM_SIZE + 5 if binary_type == "user" else CHECKSUM_SIZE + 6
    if bin_size_offset + 4 > len(data):
        return fail("binary size field exceeds file size")

    bin_size = unpack_u32(data, bin_size_offset)
    payload_size = len(data) - sign_end

    if sign_size and bin_size < sign_size:
        return fail("binary size is smaller than signing size")
    if sign_size and bin_size != sign_size + payload_size:
        return fail("binary size does not match signing area plus payload")
    if not sign_size and bin_size != payload_size:
        return fail("binary size does not match payload size")

    print("%s layout OK" % binary_type)
    print("  checksum offset : 0")
    print("  header offset   : %d" % CHECKSUM_SIZE)
    print("  header size     : %d" % header_size)
    print("  signing offset  : %d" % header_end)
    print("  payload offset  : %d" % sign_end)
    print("  payload size    : %d" % payload_size)
    return 0


def check_resource(data, sign_size):
    if len(data) < RESOURCE_HEADER_SIZE:
        return fail("resource binary is smaller than RESOURCE_HEADER_SIZE")

    header_size = unpack_u16(data, CHECKSUM_SIZE)
    bin_size = unpack_u32(data, CHECKSUM_SIZE + 6)
    sign_offset = RESOURCE_HEADER_SIZE - sign_size
    sign_end = sign_offset + sign_size

    if sign_offset < CHECKSUM_SIZE + header_size:
        return fail("resource signing area overlaps resource header fields")
    if sign_end > RESOURCE_HEADER_SIZE:
        return fail("resource signing area exceeds RESOURCE_HEADER_SIZE")

    payload_size = len(data) - RESOURCE_HEADER_SIZE
    if bin_size != payload_size:
        return fail("resource bin_size does not match RomFS payload size")

    print("resource layout OK")
    print("  checksum offset : 0")
    print("  header offset   : %d" % CHECKSUM_SIZE)
    print("  header size     : %d" % header_size)
    print("  signing offset  : %d" % sign_offset)
    print("  payload offset  : %d" % RESOURCE_HEADER_SIZE)
    print("  payload size    : %d" % payload_size)
    return 0


def main():
    parser = argparse.ArgumentParser(description="Validate signed TizenRT binary layout offsets.")
    parser.add_argument("binary", help="Path to the binary to validate")
    parser.add_argument("type", choices=["user", "common", "resource"], help="Binary type")
    parser.add_argument("--config", default=os.path.join(os.path.dirname(__file__), "..", ".config"), help="Path to .config")
    parser.add_argument("--sign-size", type=int, help="Signing prepend size")
    args = parser.parse_args()

    sign_size = get_sign_size(args)
    if sign_size is None:
        return 1
    if sign_size < 0:
        return fail("signing size must be non-negative")

    data = read_file(args.binary)
    if args.type == "resource":
        return check_resource(data, sign_size)

    return check_user_common(data, args.type, sign_size)


if __name__ == "__main__":
    sys.exit(main())
