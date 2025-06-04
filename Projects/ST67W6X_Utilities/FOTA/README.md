\mainpage Readme for FOTA tools

============
\tableofcontents

# Readme for FOTA tools
Copyright &copy; 2025 STMicroelectronics

[![ST logo](Resources/st_logo_2020.png)](https://www.st.com)

## Purpose

This folder contains tools to set a Firmware Update Over-The-Air (FOTA) environment to try out the feature.
These tools are designed to facilitate creation of a FOTA server (HTTP server) and generate a FOTA binary to exercise the feature.

## Contents

- `fota_header_gen.py`: Script for generating firmware update files with description headers and binary compression.
- `HTTP_Server.py`: HTTP server script for serving firmware updates and handling update requests.

## Requirements

- Python 3.11 or higher is recommended. The scripts may work with older versions of Python 3, but this is not guaranteed. Avoid python 2.

## Usage

### fota_header_gen.py

This script generates firmware update files with headers and compresses them.

#### Arguments

- `-i`, `--input`: Input file path to the STM32 binary (required)
- `-n`, `--st67`  : Network co processor binary file path that matches with given input file (required)
- `-t`, `--target`: Target name of the STM32(required)
- `-w`, `--firmware_type`: ST67 firmware type (required)
- `-v`, `--version`: Version of the binary (in x.y.z format) (required)
- `-c`, `--st67_version`: Version of the ST67 binary (in x.y.z format) (required)
- `-d`, `--output_dir`: Output directory path (optional)
- `-l`, `--verbose`: Enable verbose mode (optional)
- `-r`, `--board_revision`: Board revision of the STM32 (required)
- `-b`, `--prefix_board`: Target board name of the STM32 (required)

#### Usage example

Append a FOTA header on the STM32 binary and generate a json file:
```sh
python fota_header_gen.py install_header -v 1.0.0 -c 2.0.66 -t STM32U575ZI -b NUCLEO -r C05 -w NCP1 -n st67w611m_mission_t01_v2.0.75.bin.ota -i NUCLEO-U575ZI-Q/Applications/ST67W6X/ST67W6X_FOTA/STM32CubeIDE/Debug/ST67W6X_FOTA.bin
```

Generate a FOTA header source file for the STM32 code:
```sh
python fota_header_gen.py gen_header
```

### Result


### HTTP_Server.py

This script starts an HTTP/HTTPS server for Firmware Over-The-Air (FOTA) updates. It serves files over HTTP.

#### Features

- Select the files from a specified directory to expose
- Supports both HTTP and HTTPS
- Multi-threaded to handle multiple servers simultaneously (for example HTTP and HTTPS on different ports)
- IP, port, SSL certificate, SSL key and logging level among other options can be defined by user
- Graceful shutdown on termination signals (Ctrl+C)
- Supports HEAD and GET requests
- Default value so the script can be launched without arguments
- Can be called in another Python script by importing the `main_server` module
- Can be used with or without argument parser

#### Requirements

- Python 3.11 or higher is recommended. The scripts may work with older versions of Python 3, but this is not guaranteed. Avoid python 2.
- Required standard Python packages: `logging`, `threading`, `traceback`, `os`, `http.server`, `socketserver`, `json`, `sys`, `argparse`, `ssl`, `warnings`, `signal`, `concurrent.futures`, `queue`

#### Arguments

- `port`: Port to run the server on (default: 8000)
- `https-port`: Port to run the HTTPS server on (default: 8443)
- `ip`: IP address to run the server on (default: all interfaces)
- `http-version`: HTTP version to use (default: HTTP/1.1)
- `log-level`: Logging level (default: DEBUG)
- `threaded`: Enable multi-threading for the server itself to handle multiple HTTP(s) request (default: True). Does not manage the multi threading of the script that can launch multiple server in parallel (for example a HTTP server and an HTTPS server).
- `firmware-dir`: Directory where the firmware files are stored (default: ../Binaries/NCP_Binaries)
- `certfile`: Path to the SSL certificate file (default: cert.pem)
- `keyfile`: Path to the SSL key file (default: key.pem)
- `enable-https`: Enable or disable HTTPS server (default: False)

#### Usage example

```sh
python HTTP_Server.py --port 8080 --ip 127.0.0.1 --firmware-dir /path/to/firmware
```
- The server will be available at `http://127.0.0.1:8080/` with the firmware files from `/path/to/firmware` directory being expose.
- The server can be launched without arguments, in this case the default values will be used.
- It can also be called in another Python script by importing the `main_server` module.

### Known limitations
- For the FOTA header, only a .json file and a .h C file are generated

# License
See [LICENSE](LICENSE.md) for more info.
