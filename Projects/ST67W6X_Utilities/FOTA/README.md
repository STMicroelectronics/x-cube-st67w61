\mainpage Readme for FOTA tools

============
\tableofcontents

# Readme for FOTA tools
Copyright &copy; 2025 STMicroelectronics

[![ST logo](Resources/st_logo_2020.png)](https://www.st.com)


## Table of Contents

- [Readme for FOTA tools](#readme-for-fota-tools)
  - [Table of Contents](#table-of-contents)
  - [Purpose](#purpose)
  - [Contents](#contents)
  - [Workspace Structure](#workspace-structure)
  - [Requirements](#requirements)
  - [Usage](#usage)
    - [fota\_header\_gen.py](#fota_header_genpy)
      - [Arguments](#arguments)
      - [Usage Examples](#usage-examples)
      - [Output](#output)
    - [HTTP\_Server.py](#http_serverpy)
      - [Features](#features)
      - [Requirements](#requirements-1)
      - [Arguments](#arguments-1)
      - [Usage Example](#usage-example)
  - [Known Limitations](#known-limitations)
  - [License](#license)

---

## Purpose

This folder provides tools to establish a Firmware Update Over-The-Air (FOTA) environment for testing and demonstration. The tools enable easy creation of a FOTA HTTP server and generation of FOTA binaries to exercise the update feature.

---

## Contents

- **`fota_header_gen.py`**: Script to generate firmware update files with descriptive FOTA headers.  
- **`HTTP_Server.py`**: HTTP/HTTPS server script to serve firmware updates and manage update requests.

---

## Workspace Structure

| File/Folder          | Description                                 |
| -------------------- | ------------------------------------------- |
| `fota_header_gen.py` | Generates FOTA headers and related files    |
| `HTTP_Server.py`     | HTTP/HTTPS server for firmware distribution |
| `Resources/`         | Static assets (HTML, images, etc.)          |
| `README.md`          | This documentation file                     |
| `LICENSE.md`         | License information for workspace files     |

---

## Requirements

- **Python 3.11+** is recommended.  
- Older Python 3 versions may work.  
- Python 2 is **not supported**.

---

## Usage

### fota_header_gen.py

This script generates firmware update files with separate FOTA headers. It supports two subcommands:

- **`install_header`**: Generates a JSON-format FOTA header and copies the ST67W61 and STM32 binaries to the specified location.  
- **`gen_header`**: Generates a FOTA header descriptor file (`.h`) for STM32 integration.

#### Arguments

| Argument                 | Description                              | Required |
| ------------------------ | ---------------------------------------- | -------- |
| `-i`, `--input`          | Path to STM32 binary file                | Yes      |
| `-n`, `--st67`           | Path to Network Co-Processor binary file | Yes      |
| `-t`, `--target`         | Target STM32 device name                 | Yes      |
| `-w`, `--firmware_type`  | ST67 firmware type                       | Yes      |
| `-v`, `--version`        | Firmware version (format: x.y.z)         | Yes      |
| `-c`, `--st67_version`   | ST67 binary version (format: x.y.z)      | Yes      |
| `-o`, `--output_dir`     | Output directory path                    | No       |
| `-l`, `--verbose`        | Enable verbose output                    | No       |
| `-r`, `--board_revision` | STM32 board revision                     | Yes      |
| `-b`, `--prefix_board`   | STM32 target board name                  | Yes      |

#### Usage Examples

Generate a FOTA header descriptor file for STM32:

```sh
python fota_header_gen.py gen_header -o NUCLEO-U575ZI-Q/Applications/ST67W6X/ST67W6X_FOTA/Appli/App
```

Generate a JSON FOTA header and copy binaries:

```sh
python fota_header_gen.py install_header \
  -v 1.1.0 -c 2.0.89 -t STM32U575ZI -b NUCLEO -r C05 -w NCP1 \
  -n ../Binaries/NCP_Binaries/st67w611m_mission_t01_v2.0.89.bin.ota \
  -i ../Binaries/NUCLEO-U575ZI-Q_Binaries/ST67W6X_FOTA.bin
```

#### Output

- **`install_header`** generates:  
  - `.ota` file: Network Co-Processor binary (unchanged copy)  
  - `.json` file: FOTA description header  
  - `.bin` file: STM32 application binary (unchanged copy)  

- **`gen_header`** generates:  
  - `.h` header file for STM32 integration

---

### HTTP_Server.py

This script launches an HTTP/HTTPS server to distribute firmware files for FOTA.

#### Features

- Serve firmware files from a user-specified directory  
- Supports HTTP and HTTPS protocols  
- Multi-threaded to handle concurrent requests and multiple servers (e.g., HTTP and HTTPS)  
- Configurable IP, ports, SSL certificates, and logging levels  
- Graceful shutdown on termination signals (e.g., Ctrl+C)  
- Supports HTTP HEAD and GET methods   

#### Requirements

- Python 3.11+ recommended  
- Standard Python libraries: `logging`, `threading`, `traceback`, `os`, `http.server`, `socketserver`, `json`, `sys`, `argparse`, `ssl`, `warnings`, `signal`, `concurrent.futures`, `queue`

#### Arguments

| Argument         | Description                                        | Default                  |
| ---------------- | -------------------------------------------------- | ------------------------ |
| `--port`         | HTTP server port                                   | 8000                     |
| `--https-port`   | HTTPS server port                                  | 8443                     |
| `--ip`           | IP address to bind (all interfaces if unspecified) | All interfaces           |
| `--http-version` | HTTP version (e.g., HTTP/1.1)                      | HTTP/1.1                 |
| `--log-level`    | Logging verbosity level                            | DEBUG                    |
| `--threaded`     | Enable multi-threading for handling requests       | True                     |
| `--firmware-dir` | Directory containing firmware files                | ../Binaries/NCP_Binaries |
| `--certfile`     | SSL certificate file path                          | cert.pem                 |
| `--keyfile`      | SSL key file path                                  | key.pem                  |
| `--enable-https` | Enable HTTPS server                                | False                    |

#### Usage Example

Start the server on localhost port 8080 serving firmware from `STM32U575ZI_NUCLEO` directory if it exists:

```sh
python HTTP_Server.py --port 8080 --ip 127.0.0.1 --firmware-dir STM32U575ZI_NUCLEO
```

- Access the server at `http://127.0.0.1:8080/`  
- The server can be launched without arguments to use default settings  

---

## Known Limitations

- FOTA header generation currently supports only `.json` and `.h` files.

---

## License

See [LICENSE.md](LICENSE.md) for license details.

---