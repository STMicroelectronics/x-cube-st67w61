## __FOTA tools description__

This folder provides tools to establish a Firmware Update Over-The-Air (FOTA) environment for testing and demonstration. The tools enable easy creation of a FOTA HTTP server and generation of FOTA binaries to exercise the update feature.

### __Contents__

- [fota_header_gen.py](#fota_header_gen): Script to generate firmware update files with descriptive FOTA headers.
- [HTTP_Server.py](#http_server): HTTP/HTTPS server script to serve firmware updates and manage update requests.

### __Requirements__

- **Python 3.11+** is recommended.
- Standard Python libraries: `logging`, `threading`, `traceback`, `os`, `http.server`, `socketserver`, `json`, `sys`, `argparse`, `ssl`, `warnings`, `signal`, `concurrent.futures`, `queue`
- Python 2 is **not supported**.

---

### __fota_header_gen.py__ {#fota_header_gen}

This script generates firmware update files with separate FOTA headers. It supports two subcommands:

| Subcommand         | Description                                                                                              |
|--------------------|----------------------------------------------------------------------------------------------------------|
| **gen_header**     | Generates a FOTA header descriptor file (`fota_header_struct.h`) for STM32 integration.                  |
| **install_header** | Generates a JSON-format FOTA header and copies the ST67W61 and STM32 binaries to the specified location. |

```sh
python fota_header_gen.py [subcommand] [arguments]
```

##### __1. gen_header__

Files generated:

- `fota_header_struct.h`: header file for STM32 integration

| Argument                 | Description                              | Required |
| ------------------------ | ---------------------------------------- | -------- |
| `-o`, `--output_dir`     | Output directory path                    | Yes      |

Example:
```sh
python fota_header_gen.py gen_header -o NUCLEO-U575ZI-Q/Applications/ST67W6X/ST67W6X_FOTA/Appli/App
```

##### __2. install_header__

Files generated:

- `.ota`: Network Co-Processor binary (unchanged copy)
- `.json`: FOTA description header
- `.bin`: STM32 application binary (unchanged copy)

| Argument                 | Description                              | Required |
| ------------------------ | ---------------------------------------- | -------- |
| `-i`, `--input`          | Path to STM32 binary file                | Yes      |
| `-n`, `--st67`           | Path to Network Coprocessor binary file  | Yes      |
| `-t`, `--target`         | Target STM32 device name                 | Yes      |
| `-w`, `--firmware_type`  | ST67 firmware type                       | Yes      |
| `-v`, `--version`        | Firmware version (format: x.y.z)         | Yes      |
| `-c`, `--st67_version`   | ST67 binary version (format: x.y.z)      | Yes      |
| `-o`, `--output_dir`     | Output directory path                    | No       |
| `-l`, `--verbose`        | Enable verbose output                    | No       |
| `-r`, `--board_revision` | STM32 board revision                     | Yes      |
| `-b`, `--prefix_board`   | STM32 target board name                  | Yes      |

Example:

```sh
python fota_header_gen.py install_header -v 1.3.0 -c 2.0.106 -t STM32U575ZI -b NUCLEO -r C05 -w NCP1 -n ../Binaries/NCP_Binaries/st67w611m_mission_t01_v2.0.106.bin.ota -i ../Binaries/NUCLEO-U575ZI-Q_Binaries/ST67W6X_FOTA.bin
```

---

### __HTTP_Server.py__ {#http_server}

This script launches an HTTP/HTTPS server to distribute firmware files for FOTA.

- Serve firmware files from a user-specified directory
- Supports HTTP and HTTPS protocols
- Multi-threaded to handle concurrent requests and multiple servers (e.g., HTTP and HTTPS)
- Configurable IP, ports, SSL certificates, and logging levels
- Graceful shutdown on termination signals (e.g., Ctrl+C)
- Supports HTTP HEAD and GET methods

```sh
python HTTP_Server.py [arguments]
```

##### __Arguments__

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

Example:

Start the server on localhost port 8080 serving firmware from `STM32U575ZI_NUCLEO` directory if it exists:

```sh
python HTTP_Server.py --port 8080 --ip 127.0.0.1 --firmware-dir STM32U575ZI_NUCLEO
```

Access the server at `http://127.0.0.1:8080/`

### __License__

See [LICENSE.md](LICENSE.md) for license details.
