#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# Verified with pycodestyle option --max-line-length=160

import argparse
import os

file_header = '''/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    html_pages.h
  * @author  ST67 Application Team
  * @brief   index.html conversion in hex array
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
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef HTML_PAGES_H
#define HTML_PAGES_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
'''

file_footer = '''
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* HTML_PAGES_H */
'''

# Server name
http_header_server = "Server: U5"

# HTTP Response code
http_ok = 'HTTP/1.1 200 OK'
http_not_found = 'HTTP/1.1 404 File not found'

# HTTP Content
http_content_type = 'Content-Type: '
http_content_len = 'Content-Length: '

# HTTP Control
http_header_ctrl_origin = 'Access Control-Allow-Origin: * '
http_header_ctrl_method = 'Access Control-Allow-Methods: '
http_header_ctrl_head = 'Access Control-Allow-Headers: Cache-Control, Last-Event-Id, X-Requested-With'
http_header_ctrl_cache = 'Cache-Control: no-cache'

# HTTP Method
http_method_get = 'GET'
http_method_post = 'POST'
http_method_head = 'HEAD'
http_method_put = 'PUT'
http_method_delete = 'DELETE'

# HTTP Type
http_type_html = 'text/html; charset=utf8'
http_type_svg = 'image/svg+xml'

# HTTP Connection
http_header_conn_close = 'Connection: close'
http_header_conn_keep_alive = 'Connection: keep-alive'

# Examples of headers
html_default_header = [http_ok,
                       http_header_server,
                       http_header_ctrl_origin,
                       http_header_ctrl_method + http_method_get,
                       http_header_ctrl_head,
                       http_header_ctrl_cache,
                       http_content_len,
                       http_content_type + http_type_html]

svg_default_header = [http_ok,
                      http_header_server,
                      http_header_conn_close,
                      http_header_ctrl_cache,
                      http_content_len,
                      http_content_type + http_type_svg]

# HTML files descriptor.
# the key is the filename
# the value is a list of [<label variable used in code>, <ordered http header elements>]
html_desc_dict = {
    'index_httpserver.html':                    ['response_index_html',
                                                 html_default_header],
    '404.html':                                 ['response_error_404_html',
                                                 [http_not_found,
                                                  http_header_server,
                                                  http_header_conn_close,
                                                  http_content_len,
                                                  http_content_type + http_type_html]
                                                 ],
    'favicon.svg':                              ['response_favicon_svg',
                                                 svg_default_header],
    'ST_logo_2020_white_no_tagline_rgb.svg':    ['response_st_logo_svg',
                                                 svg_default_header],
}

# Initialize the argument parser
parser = argparse.ArgumentParser(formatter_class=argparse.RawTextHelpFormatter)
parser.add_argument("--config", required=True, help="configuration file containing list of files to convert")
parser.add_argument("--output", default='', help="Path to the header file")
args = parser.parse_args()

if not os.path.exists(args.config):
    print('config file not found')
    exit(1)

# Get the elements list to convert in byte-array
with open(args.config, 'r') as f:
    files_to_convert = f.readlines()

# Get the current directory
input_dir = os.path.abspath(os.path.dirname(args.config))

# Select the output directory. current dir selected if not argument defined
output_dir = args.output if args.output else input_dir

# Prepare the html_pages.h with header section
index_hex_str = file_header

for e in files_to_convert:
    # Get the filename of the element to convert
    file = e.strip()
    input_path = os.path.join(input_dir, file)

    # Check if file exists and is defined in dict
    if not os.path.exists(input_path):
        print(f'{file} not found')
        exit(1)
    elif html_desc_dict.get(file, None) is None:
        print(f'{file} not described')
        exit(1)

    # Read the file content as binary with hexadecimal format
    with open(input_path, 'rb') as f:
        str_hex = f.read().hex()

    label, header_elements = html_desc_dict[file]
    # Add the first part of the element descriptor
    index_hex_str += f'\nconst char {label}[] =\n{{\n'
    # Add the all part of the element descriptor
    for elmt in header_elements:
        # If content-length element, add the length of the input file
        if 'Content-Len' in elmt:
            index_hex_str += '  ' + ''.join([f"'{x}', " for x in elmt])
            index_hex_str += ', '.join([hex(ord(c)) for c in str(int(len(str_hex)/2))])
            index_hex_str += ", '\\r', '\\n',\n"
        else:
            index_hex_str += '  ' + ''.join([f"'{x}', " for x in elmt]) + "'\\r', '\\n',\n"

    # Add final new line characters to declare the end of header
    index_hex_str += "  '\\r', '\\n',\n "

    # Add the element content as hexadecimal format
    # Add new line after 18 characters
    # Add null character at the end
    index_hex_str += ','.join([f' 0x{str_hex[i:i+2]}'
                               if i == 0 or i % 36 else f'\n  0x{str_hex[i:i+2]}'
                               for i in range(0, len(str_hex), 2)])
    index_hex_str += ', 0x00'
    # Finalize element structure
    index_hex_str += '\n};\n'

# Finalize the html_pages.h with footer section
index_hex_str += file_footer

# Write the html_pages.h
if not os.path.exists(output_dir):
    os.makedirs(output_dir)

with open(os.path.join(output_dir, 'html_pages.h'), 'w') as f:
    f.write(index_hex_str)
