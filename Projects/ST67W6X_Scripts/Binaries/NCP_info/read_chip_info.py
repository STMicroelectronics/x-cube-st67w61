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
import argparse
import struct


# =====================================================
def read_efuse_data(file_path):
    try:
        with open(file_path, 'rb') as file:
            # Read the binary data from 0x00 to 0x1FF
            efuse_data = file.read(0x200)
            return efuse_data
    except FileNotFoundError:
        print(f"File not found: {file_path}")
        return None
    except Exception as e:
        print(f"An error occurred: {e}")
        return None


# =====================================================
def extract_bits(value, start_bit, end_bit):
    mask = (1 << (end_bit - start_bit + 1)) - 1
    return (value >> start_bit) & mask


# =====================================================
def most_significant_bit(value):
    msb = 0
    while value > 0:
        value >>= 1
        msb += 1
    return msb


# =====================================================
def parse_efuse_data(efuse_data):
    efuse_description = {
        0x00: {"JTAG": (27, 26)},
        0x1C: ("Public Key", 32, 0),
        0x7C: {"Anti-rollback": 12},
        0x170: ("Anti-rollback Bootloader", 16, 0),
        0x180: ("Anti-rollback Application", 32, 0),
        0x14: ("Default MAC Address", 6, 0),
        0x64: ("Customer MAC Address1", 6, 0),
        0x70: ("Customer MAC Address2", 6, 0),
        0x100: ("Part Number", 24, 1),
        0x11A: ("Manufacturing Year/Week", 2, 0),
        0x118: ("BOM Id", 2, 1),
    }

    trim_table = [
        {"en_addr": 0xCC, "en_offset": 26, "value_addr": 0xC0, "value_offset": 0, "value_len": 15, "parity_addr": 0xC0, "parity_offset": 15, "type": 0, "desc": "wifi_hp_poffset0"},
        {"en_addr": 0xCC, "en_offset": 27, "value_addr": 0xC0, "value_offset": 16, "value_len": 15, "parity_addr": 0xC0, "parity_offset": 31, "type": 0, "desc": "wifi_hp_poffset1"},
        {"en_addr": 0xCC, "en_offset": 28, "value_addr": 0xC4, "value_offset": 0, "value_len": 15, "parity_addr": 0xC4, "parity_offset": 15, "type": 0, "desc": "wifi_hp_poffset2"},
        {"en_addr": 0xCC, "en_offset": 29, "value_addr": 0xC4, "value_offset": 16, "value_len": 15, "parity_addr": 0xC4, "parity_offset": 31, "type": 1, "desc": "wifi_lp_poffset0"},
        {"en_addr": 0xCC, "en_offset": 30, "value_addr": 0xC8, "value_offset": 0, "value_len": 15, "parity_addr": 0xC8, "parity_offset": 15, "type": 1, "desc": "wifi_lp_poffset1"},
        {"en_addr": 0xCC, "en_offset": 31, "value_addr": 0xC8, "value_offset": 16, "value_len": 15, "parity_addr": 0xC8, "parity_offset": 31, "type": 1, "desc": "wifi_lp_poffset2"},
        {"en_addr": 0xD0, "en_offset": 26, "value_addr": 0xCC, "value_offset": 0, "value_len": 25, "parity_addr": 0xCC, "parity_offset": 25, "type": 2, "desc": "ble_poffset0"},
        {"en_addr": 0xD0, "en_offset": 27, "value_addr": 0xD0, "value_offset": 0, "value_len": 25, "parity_addr": 0xD0, "parity_offset": 25, "type": 2, "desc": "ble_poffset1"},
        {"en_addr": 0xD0, "en_offset": 28, "value_addr": 0xD4, "value_offset": 0, "value_len": 25, "parity_addr": 0xD4, "parity_offset": 25, "type": 2, "desc": "ble_poffset2"},
        {"en_addr": 0xEC, "en_offset": 7, "value_addr": 0xEC, "value_offset": 0, "value_len": 6, "parity_addr": 0xEC, "parity_offset": 6, "type": 3, "desc": "xtal0"},
        {"en_addr": 0xF0, "en_offset": 31, "value_addr": 0xF4, "value_offset": 26, "value_len": 6, "parity_addr": 0xF0, "parity_offset": 30, "type": 3, "desc": "xtal1"},
        {"en_addr": 0xEC, "en_offset": 23, "value_addr": 0xF4, "value_offset": 20, "value_len": 6, "parity_addr": 0xF0, "parity_offset": 28, "type": 3, "desc": "xtal2"}
    ]

    parsed_data = {}
    for addr, desc in efuse_description.items():
        if isinstance(desc, dict):
            for field, bit_range in desc.items():
                if isinstance(bit_range, tuple):
                    start_bit, end_bit = bit_range
                    value = extract_bits(struct.unpack_from('<I', efuse_data, addr)[0], end_bit, start_bit)
                else:
                    value = (struct.unpack_from('<I', efuse_data, addr)[0] >> bit_range) & 1
                if 'JTAG' in field:
                    parsed_data[field] = 'Disabled' if value == 3 else 'Enabled'
                elif 'Anti-rollback' in field:
                    parsed_data[field] = 'Enabled' if value == 1 else 'Disabled'
                else:
                    parsed_data[field] = value
        else:
            field, size, order = desc
            if order == 0:
                parsed_data[field] = efuse_data[addr:addr + size][::-1].hex().upper()
            else:
                parsed_data[field] = efuse_data[addr:addr + size].hex().upper()

            if 'MAC Address' in field:
                # split the hex string as byte array (2 characters)
                data_lst = [parsed_data[field][i:i + 2] for i in range(0, len(parsed_data[field]), 2)]
                # add the colon separator
                parsed_data[field] = ':'.join(data_lst)
            elif 'Anti-rollback' in field:
                # split the parsed data: the data are redundant (first half is the same as the second half)
                parsed_data[field] = parsed_data[field][:size]
                parsed_data[field] = int(parsed_data[field], 16)
                parsed_data[field] = most_significant_bit(parsed_data[field])
            elif 'Part Number' in field:
                # split the hex string as byte array (2 characters)
                data_lst = [parsed_data[field][i:i + 2] for i in range(0, len(parsed_data[field]), 2)]
                # search if 0x03 is in the list
                if '03' in data_lst:
                    # remove the stop byte (0x03) and next bytes
                    data_lst = data_lst[:data_lst.index('03')]
                    # convert the hex string to a list of bytes
                    parsed_data[field] = ''.join([bytes.fromhex(data).decode('ascii') for data in data_lst])
                else:
                    parsed_data[field] = 'Invalid Part Number'
            elif 'Manufacturing' in field:
                # convert the hex to decimal: First byte is the week, second byte is the year
                parsed_data[field] = int(parsed_data[field], 16)
                week = (parsed_data[field] >> 8) & 0xFF
                year = parsed_data[field] & 0xFF
                parsed_data[field] = f"Year 20{year:02} | Week {week:02}"
            elif 'Public Key' in field:
                # return only if the key is filled or not
                parsed_data[field] = 'Filled' if parsed_data[field] != '00' * 32 else 'Empty'

    # Process trimming values
    for trim in trim_table:
        en_value = struct.unpack_from('<I', efuse_data, trim["en_addr"])[0]
        if (en_value >> trim["en_offset"]) & 0x1:
            parity_value = struct.unpack_from('<I', efuse_data, trim["parity_addr"])[0]
            trim_value = struct.unpack_from('<I', efuse_data, trim["value_addr"])[0]
            trim_parity = (parity_value >> trim["parity_offset"]) & 0x1
            trim_bits = (trim_value >> trim["value_offset"]) & ((1 << trim["value_len"]) - 1)

            # Calculate parity
            cnt = bin(trim_bits).count('1')
            if (cnt & 0x1) == trim_parity:
                if trim["type"] == 2:  # BLE
                    parsed_data[trim["desc"][:-1]] = [((trim_bits >> (i * 5)) & 0x1F) - 32 if ((trim_bits >> (i * 5)) & 0x1F) >= 16 else (trim_bits >> (i * 5)) & 0x1F for i in range(5)]
                elif trim["type"] in [0, 1]:  # Wi-Fi
                    pwr_offset_tmp = [((trim_bits >> (i * 5)) & 0x1F) - 32 if ((trim_bits >> (i * 5)) & 0x1F) >= 16 else (trim_bits >> (i * 5)) & 0x1F for i in range(3)]
                    pwr_offset = [0] * 14
                    pwr_offset[0] = pwr_offset_tmp[0]
                    pwr_offset[6] = pwr_offset_tmp[1]
                    pwr_offset[12] = pwr_offset_tmp[2]

                    step = (pwr_offset_tmp[1] - pwr_offset_tmp[0]) * 100 // 6
                    for k in range(1, 6):
                        pwr_offset[k] = ((step * k) + 50) // 100 + pwr_offset_tmp[0]

                    step = (pwr_offset_tmp[2] - pwr_offset_tmp[1]) * 100 // 6
                    for k in range(7, 12):
                        pwr_offset[k] = ((step * (k - 6)) + 50) // 100 + pwr_offset_tmp[1]

                    pwr_offset[13] = (step * 7 + 50) // 100 + pwr_offset_tmp[1]

                    parsed_data[trim["desc"][:-1]] = pwr_offset
                elif trim["type"] == 3:  # XTAL
                    parsed_data[trim["desc"][:-1]] = trim_bits

    return parsed_data


# =====================================================
# Main method
# =====================================================
def main():
    parser = argparse.ArgumentParser(description='Read and parse e-Fuse data from a binary file.')
    parser.add_argument('file_path', nargs='?', default='default_binary_file.bin', help='Path to the binary file')
    args = parser.parse_args()

    efuse_data = read_efuse_data(args.file_path)

    if efuse_data:
        parsed_data = parse_efuse_data(efuse_data)
        print("Parsed e-Fuse Data (0x00 to 0x1FF):")
        for field, value in parsed_data.items():
            print(f"{field:30}: {value}")


if __name__ == "__main__":
    main()
    exit(0)
