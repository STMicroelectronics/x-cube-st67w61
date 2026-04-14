#! /usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Copyright (c) 2025 STMicroelectronics.
# All rights reserved.
#
# This software is licensed under terms that can be found in the LICENSE file
# in the root directory of this software component.
# If no LICENSE file comes with this software, it is provided AS-IS.

# =====================================================
# Imports
# =====================================================
import datetime
import logging
import os
import shutil
import sys
import argparse
import hashlib
import json
import warnings
import zlib
from typing import IO
import struct

# =====================================================
# Global variables
# =====================================================
# Constants for the FOTA header
MAGIC_NUMBER = "ST67W611"  # Magic number
FOTA_HEADER_SIZE = 512  # Example header size for the C structure
FOTA_PROTOCOL_VERSION = '1.0.0'

RESERVED_VALUE = b'\xFF'  # Example reserved value
NEW_BIN_PREFIX = "fota_"  # Prefix for the new binary file
JSON_PREFIX = "ST67W611"  # Prefix for the json file
FOTA_HEADER_STRUCT_FILE_NAME = "fota_header_struct.h"

# Structure naming
MAGIC_NUMBER_STRING = 'magic_num'
HEADER_VER = 'protocol_version'
HEADER_SIZE = 'header_size'
STM32_APP_BIN_VER = 'stm32_app_ver'
ST67_VER = 'st67_ver'
DATA_TYPE = 'data_type'
FW_TYPE = 'firmware_type'
DESC = 'desc'
RESERVED = 'reserved'
FILE_HASH = 'file_hash'
START_ADDRESS = 'start_address'
PREFIX_BOARD_NAME = 'prefix_board_name'
BOARD_REV = 'board_revision'
INFO_FIELDS = 'info'

# Initial header data for json file
HEADER_FIELDS_JSON = {
    f"{MAGIC_NUMBER_STRING}": MAGIC_NUMBER,
    f"{HEADER_VER}": FOTA_PROTOCOL_VERSION,
    f"{INFO_FIELDS}": {
        f"{FW_TYPE}": " ",
        f"{DATA_TYPE}": " ",
        f"{PREFIX_BOARD_NAME}": " ",
        f"{BOARD_REV}": " ",
        f"{DESC}": "FOTA firmware",
    },
    f"{STM32_APP_BIN_VER}": " ",
    f"{ST67_VER}": " ",
    f"{FILE_HASH}": " ",
}

# Define the header fields and their types
# how it works :
# On the right side of the colon, there is the value field containing the size of the value and its type.
# Python will manage types for the bin header generation but for the .h generation, it needs to be translated into C types
# 8s = 8 bytes char array, uint8_t deadbeaf[8] (s stands for uint8_t )
# To update the structure, this dictionary and header_values need to be edited in create_header_c function
HEADER_FIELDS_C = {
    f"{MAGIC_NUMBER_STRING}": "16s",
    f"{HEADER_VER}": "16s",
    f"{DATA_TYPE}": "16s",
    f"{PREFIX_BOARD_NAME}": "16s",
    f"{BOARD_REV}": "16s",
    f"{FW_TYPE}": "16s",
    f"{STM32_APP_BIN_VER}": "16s",
    f"{ST67_VER}": "16s",
    # f"{HEADER_SIZE}": "I",
    # f"{START_ADDRESS}": "I",
    f"{FILE_HASH}": "32s",
    f"{RESERVED}": "s"  # Placeholder for reserved field
}

# Reminder of formatting in python
# format_spec     ::=  [[fill]align][sign]["z"]["#"]["0"][width][grouping_option]["." precision][type]
# fill            ::=  <any character>
# align           ::=  "<" | ">" | "=" | "^"
# sign            ::=  "+" | "-" | " "
# width           ::=  digit+
# grouping_option ::=  "_" | ","
# precision       ::=  digit+
# type            ::=  "b" | "c" | "d" | "e" | "E" | "f" | "F" | "g" | "G" | "n" | "o" | "s" | "x" | "X" | "%"

# from https://docs.python.org/3/library/string.html#format-specification-mini-language

# =====================================================
# Create a local logger for this module
logger = logging.getLogger(__name__)


def configure_local_logging(log_level, log_file=None):
    numeric_level = getattr(logging, log_level.upper(), None)
    if not isinstance(numeric_level, int):
        raise ValueError(f'Invalid log level: {log_level}')
    formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')

    # Stream handler (console)
    stream_handler = logging.StreamHandler()
    stream_handler.setFormatter(formatter)

    # File handler (optional)
    handlers: list[logging.Handler] = [stream_handler]
    if log_file:
        file_handler = logging.FileHandler(log_file, mode='a', encoding='utf-8')
        file_handler.setFormatter(formatter)
        handlers.append(file_handler)

    logger.setLevel(numeric_level)
    # Remove old handlers to avoid duplicate logs
    logger.handlers = []
    for h in handlers:
        logger.addHandler(h)


# =====================================================
# Create a C type string based on the field type
def get_c_type(field_type: str) -> str:
    """Determine the C type based on the field type."""
    return "uint8_t" if "s" in field_type else "uint32_t"


# =====================================================
# Create a C string variable declaration based on the field type
def get_c_variable(field: str, field_type: str, c_type: str, size: str) -> str:
    """Create a C variable declaration based on the field type."""
    return f"  {c_type} {field}[{size}];\n" if "s" in field_type else f"  {c_type} {field};\n"


# =====================================================
# Create bytes array in C codding format based on header values provided
def create_header_c(target_name: str, firmware_type: str, version: str, st67_version: str, file_raw: bytes) -> bytes:
    """Create a binary header using C structure."""

    # Calculate the size of the other fields
    other_fields_size = struct.calcsize(' '.join(v for k, v in HEADER_FIELDS_C.items() if v is not None and k is not RESERVED))
    # Calculate the reserved size
    reserved_size = FOTA_HEADER_SIZE - other_fields_size
    HEADER_FIELDS_C[RESERVED] = f"{reserved_size}s"
    # Create the header format string
    header_format = ' '.join(HEADER_FIELDS_C.values())
    header_values = {
        f"{MAGIC_NUMBER_STRING}": MAGIC_NUMBER.encode('utf-8'),
        f"{HEADER_VER}": FOTA_PROTOCOL_VERSION.encode('utf-8'),
        f"{DATA_TYPE}": target_name.encode('utf-8'),
        f"{FW_TYPE}": firmware_type.encode('utf-8'),
        f"{STM32_APP_BIN_VER}": version.encode('utf-8'),
        f"{ST67_VER}": st67_version.encode('utf-8'),
        f"{HEADER_SIZE}": FOTA_HEADER_SIZE,
        f"{START_ADDRESS}": len(file_raw),
        f"{FILE_HASH}": calculate_sha256_hash(file_raw),
        f"{RESERVED}": RESERVED_VALUE * reserved_size
    }
    logger.debug(f"Header values: {header_values}")

    # Pack the data into a binary format
    header = struct.pack(
        header_format,
        *header_values.values()
    )
    return header


# =====================================================
# Process nested dictionary for .h generation
def generate_defines_recursive(d, parent_key=""):
    """Recursively generate #define directives for nested dictionaries."""
    defines = ""
    for key, value in d.items():
        full_key = f"{parent_key}_{key}".upper() if parent_key else key.upper()
        if isinstance(value, dict):
            # Recurse into sub-dictionaries
            defines += generate_defines_recursive(value, full_key)
        else:
            # Generate #define directive
            defines += f"#define {full_key:30}  \"{key}\"\n"
    return defines


# =====================================================
def to_json_bytes(obj: dict) -> bytes:
    return json.dumps(obj, indent=None, ensure_ascii=True, separators=(',', ':')).encode('utf-8')


# =====================================================
def open_binary_file(file_path: str) -> IO:
    """Open a binary file in read/write mode if it exists."""
    try:
        if os.path.exists(file_path):
            return open(file_path, 'r+b')
        else:
            logger.error(f"File does not exist: {file_path}")
            raise FileNotFoundError
    except OSError as e_open_bin:
        logger.error(f"Error opening file: {e_open_bin}")
        raise


# =====================================================
def create_binary_file(file_path: str) -> IO:
    """Create a binary file, overwriting it if it already exists."""
    try:
        return open(file_path, 'wb+')
    except OSError as e_create_bin:
        logger.error(f'Error creating/opening file: {e_create_bin}')
        raise


# =====================================================
def is_power_of_two(n: int) -> bool:
    """Check if a number is a power of two."""
    return (n & (n - 1) == 0) and n != 0


# =====================================================
def calculate_sha256_hash(data: bytes) -> bytes:
    """Calculate the SHA-256 hash of the given data and return the value in bytes format."""
    try:
        sha256 = hashlib.sha256()
        sha256.update(data)
        logger.debug(f'sha256 is : {sha256.digest()}')
        return sha256.digest()
    except Exception as e_sha256:

        logger.error(f'Error calculating SHA-256 hash: {e_sha256}')
        raise


# =====================================================
def calculate_crc32(data: bytes) -> str:
    """Calculate the CRC32 of the given data and return the value in unsigned 32-bit integer format."""
    try:
        crc32 = zlib.crc32(data)
        logger.debug(f"CRC32 is : {crc32}")
        return f"{crc32}"
    except Exception as e_crc32:

        logger.error(f"Error calculating CRC32: {e_crc32}")
        raise


# =====================================================
def pad_header(header: bytes) -> bytes:
    """Pad the header to the FOTA header size."""
    try:
        padded_header = bytearray(header)
        padded_header.extend(RESERVED_VALUE * (FOTA_HEADER_SIZE - len(padded_header)))
        return padded_header
    except Exception as e_pad:
        logger.error(f'Error padding header: {e_pad}')
        raise


# =====================================================
def create_header_json(target_name: str, prefix_board_name: str, board_revision: str, firmware_type: str, version: str, st67_version: str, file_raw: bytes) -> bytes:
    """Create a JSON header."""
    try:
        HEADER_FIELDS_JSON[INFO_FIELDS][DATA_TYPE] = target_name
        HEADER_FIELDS_JSON[INFO_FIELDS][FW_TYPE] = firmware_type
        HEADER_FIELDS_JSON[FILE_HASH] = list(calculate_sha256_hash(file_raw))
        HEADER_FIELDS_JSON[INFO_FIELDS][PREFIX_BOARD_NAME] = prefix_board_name
        HEADER_FIELDS_JSON[STM32_APP_BIN_VER] = version
        HEADER_FIELDS_JSON[ST67_VER] = st67_version
        HEADER_FIELDS_JSON[INFO_FIELDS][BOARD_REV] = board_revision
        # Convert header data to JSON bytes
        header_bytes = to_json_bytes(HEADER_FIELDS_JSON)

        return header_bytes
    except (OSError, json.JSONDecodeError, KeyError) as e_create_header_json:
        logger.error(f'Error creating JSON header: {e_create_header_json}')
        raise


# =====================================================
def add_fota_header(target_name: str, prefix_board_name: str, board_revision: str, firmware_type: str, file_raw: bytes, file_name: str,
                    format_header: str, output_dir, version: str, st67_version: str) -> None:
    """Add an FOTA header to the binary file."""
    try:
        # Because the -f -format argument is in an experimental state , default is json format
        if format_header.lower() == "c":
            header_bytes = create_header_c(target_name, firmware_type, version, st67_version, file_raw)

            padded_header = pad_header(header_bytes)
            # Create the final output with padded header and raw data
            output_data = bytearray(padded_header)

            output_data.extend(file_raw)

        elif format_header.lower() == "json":
            file_pointer = create_binary_file(os.path.join(output_dir, f"{JSON_PREFIX}_{target_name}_{prefix_board_name}.json"))
            header_bytes_j = create_header_json(target_name, prefix_board_name, board_revision, firmware_type, version, st67_version, file_raw)
            file_pointer.write(header_bytes_j)
            file_pointer.close()

            output_data = bytearray(file_raw)

        elif format_header.lower() == "both":
            header_bytes = create_header_c(target_name, firmware_type, version, st67_version, file_raw)
            file_pointer = create_binary_file(os.path.join(output_dir, f"{JSON_PREFIX}_{target_name}.json"))
            header_bytes_j = create_header_json(target_name, prefix_board_name, board_revision, firmware_type, version, st67_version, file_raw)
            file_pointer.write(header_bytes_j)
            file_pointer.close()

            padded_header = pad_header(header_bytes)
            # Create the final output with padded header and raw data
            output_data = bytearray(padded_header)

            output_data.extend(file_raw)
        else:
            raise ValueError("Invalid format specified")

        # Write the uncompressed output to a new file
        output_file_path_uncompressed = os.path.join(output_dir, f"{NEW_BIN_PREFIX}{file_name}")
        with open(output_file_path_uncompressed, "wb+") as fota_file:
            fota_file.write(output_data)

    except (OSError, json.JSONDecodeError, KeyError) as e_header:
        logger.error(f"Error adding FOTA header: {e_header}")
        raise


# =====================================================
def do_header_gen(args: argparse.Namespace):
    """Main function to execute the script."""
    if args.format != 'json':
        raise ValueError("Format is not json, only json format is supported for now")

    logger.debug(f'Input file: {args.input}')
    logger.debug(f'Output directory: {args.output_dir}')
    logger.debug(f'Format selected is: {args.format}')
    logger.debug(f'Version is: {args.version}')
    logger.debug(f'ST67 version is: {args.st67_version}')

    try:
        # Create or open the binary file
        file_pointer = open_binary_file(args.input)
        file_raw = file_pointer.read()
        file_pointer.close()

        # Create a subdirectory for each target within the output directory
        target_output_dir = os.path.join(args.output_dir, f"{args.target}_{args.prefix_board}")
        os.makedirs(target_output_dir, exist_ok=True)

        # Copy the ST67 file to the output directory if it exists
        if args.st67 is not None:
            if os.path.exists(args.st67):
                shutil.copy(str(args.st67), str(target_output_dir))
            else:
                logger.warning(f"ST67 file does not exist: {args.st67}")

        # Create the output file path
        # Strip the input path to get only the file name with extension
        file_name_with_extension = os.path.basename(args.input)

        # Add FOTA header to the binary file
        add_fota_header(args.target, args.prefix_board, args.board_revision, args.firmware_type, file_raw, file_name_with_extension, args.format, target_output_dir, args.version, args.st67_version)

    except Exception as e_gen:
        logger.error(f'Error in main execution: {e_gen}')
        raise


# =====================================================
def do_header_c_gen(args: argparse.Namespace):
    """Generate a .h header file with the FOTA header structure."""
    header_content = f'''/**
  ******************************************************************************
  * @file    fota_header_struct.h
  * @author  ST67 Application Team
  * @brief   FOTA header structure definition
  * @note    Auto generated file, DO NOT MODIFY
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025-2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef FOTA_HEADER_STRUCT_H
#define FOTA_HEADER_STRUCT_H

#ifdef __cplusplus
extern "C" {{
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported constants --------------------------------------------------------*/
#define MAGIC_NUMBER                    "{MAGIC_NUMBER}"
#define FOTA_PROTOCOL_VERSION           "{FOTA_PROTOCOL_VERSION}"

'''
    file_path = os.path.join(args.output_dir, FOTA_HEADER_STRUCT_FILE_NAME)

    # Add #define directives using the recursive function
    header_content += generate_defines_recursive(HEADER_FIELDS_JSON)
    header_content += """\n/* Exported types ------------------------------------------------------------*/\ntypedef struct\n{\n"""
    for field, field_type in HEADER_FIELDS_C.items():
        if field is RESERVED:
            continue
        c_type = get_c_type(field_type)
        size = field_type.replace("s", "")
        header_content += get_c_variable(field, field_type, c_type, size)
    header_content += """} FotaHeader_t;\n\n"""
    header_content += "#ifdef __cplusplus\n}\n#endif /* __cplusplus */\n\n#endif /* FOTA_HEADER_STRUCT_H */\n"""

    try:
        with open(file_path, 'w') as header_file:
            header_file.write(header_content)
        logger.debug(f'Output directory: {args.output_dir}')
        logger.debug(f'Header content generated:\n{header_content}')
        logger.debug(f"Header file '{file_path}' generated successfully.")
    except OSError as e_c_header:
        logger.error(f'Error generating header file: {e_c_header}')


# =====================================================
# Subcommands global mapping
subcmds = {
    'install_header': do_header_gen,
    'gen_header': do_header_c_gen,
}


# =====================================================
# Main function
# =====================================================
def main():
    parser = argparse.ArgumentParser(description='A simple argument parser example.')

    # Adding arguments
    subs = parser.add_subparsers(help='subcommand help', dest='subcmd')
    install_head = subs.add_parser('install_header', help='Install header to an existing binary')
    install_head.add_argument('-i', '--input', type=str, required=True, help='Input file path to the STM32 binary')
    install_head.add_argument('-n', '--st67', type=str, required=True, help='Network co processor binary file path that matches with given input file')
    install_head.add_argument('-t', '--target', type=str, required=True, help='Target name of the STM32')
    install_head.add_argument('-r', '--board_revision', type=str, required=True, help='Board revision of the STM32')
    install_head.add_argument('-b', '--prefix_board', type=str, required=True, help='Target board name of the STM32')
    install_head.add_argument('-w', '--firmware_type', type=str, required=True, help='ST67 firmware type')
    install_head.add_argument('-o', '--output_dir', type=str, default=os.path.dirname(__file__), help='Output directory path')
    install_head.add_argument('-f', '--format', type=str, default='json', help='Choose format of version either json, c or type "both" to add both the binary')
    install_head.add_argument('-v', '--version', type=str, required=True, help='Version of the STM32 binary (in x.y.z format)')
    install_head.add_argument('-c', '--st67_version', type=str, required=True, help='Version of the ST67 binary (in x.y.z format)')
    install_head.add_argument('-l', '--verbose', action='store_true', help='Enable verbose mode')

    gen_head = subs.add_parser('gen_header', help='Generate header c file for the FOTA header structure')
    gen_head.add_argument('-o', '--output_dir', type=str, default=os.path.dirname(__file__), help='Output directory path')
    gen_head.add_argument('-l', '--verbose', action='store_true', help='Enable verbose mode')

    args = parser.parse_args()
    if args.subcmd is None:
        logger.error('Must specify a subcommand')
        sys.exit(1)

    # Build a unique log file name using subparser and timestamp
    now = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    log_file_name = f"log_fota_header_gen_{args.subcmd}_{now}.txt"
    log_file_path = os.path.join(os.getcwd(), log_file_name)

    configure_local_logging(getattr(args, "log_level", "DEBUG"), log_file_path)

    subcmds[args.subcmd](args)


# =====================================================
if __name__ == '__main__':
    configure_local_logging("DEBUG")
    logger.info('Starting the script...')
    logger.debug(
        "This script doesn't require external dependencies outside of Python builtins and the standard library.")

    # Check Python version
    if sys.version_info[0] < 3:
        raise Exception('Must be using Python 3')
    try:
        main()
        logger.info("Script was executed successfully.")
    except Exception as e:
        logger.error(f"Script execution failed: {e}")
