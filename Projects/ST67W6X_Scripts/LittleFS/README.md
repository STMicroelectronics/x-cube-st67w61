## __Certificate and Little File System description__

Scripts to generate certificates and keys and to convert them into a LittleFS binary image.

> [!IMPORTANT]
> The `littlefs/littlefs.bin` binary is required to load the NCP firmware on the ST67W611M Network Coprocessor.

### __Contents__

- [build.bat](#buildbat): Script to generate the LittleFS binary image containing certificates and keys.
- [gen_mqtt_certificate.py](#gen_mqtt_certificatepy): Python script to generate self-signed MQTT certificates and private keys for testing.

---

### __build.bat__ {#buildbat}

The NCP uses LittleFS to read certificates and keys from the host processor, which are then stored internally.

1. Place your certificates and keys in the `Certificates/lfs` folder.
2. Run the **`build.bat`** script to generate the `littlefs/littlefs.bin` file. This binary file contains the LittleFS image with your certificates and keys.

> [!NOTE]
> The `littlefs.bin` file is used during the NCP firmware flashing process described in the `../Binaries/README.md`.

---

### __gen_mqtt_certificate.py__ {#gen_mqtt_certificatepy}

This Python script generates a self-signed MQTT certificate and private key pair for testing purposes.

1. Ensure you have Python installed on your system.
2. Run the **`gen_mqtt_certificate.py`** script. This creates self-signed certificates and keys in the `Certificates/lfs` folder.
3. The script also generates a C header template in `output/mqtt_certificate_template.h` containing:
  - `#define MQTT_SECURITY_LEVEL`
  - `#define MQTT_HOST_PORT` (1883 for security level 0, otherwise 8883)
  - `#define MQTT_CERTIFICATE`
  - `#define MQTT_KEY`
  - `#define MQTT_CA_CERTIFICATE`
  - `#define MQTT_SNI` (`""` for security levels 0 and 1)
  - `static const char ca_certificate[]`
  - `static const char client_certificate[]`
  - `static const char client_key[]`
4. The script generates 3 Mosquitto configuration files in `output/`:
  - `mosquitto_0.conf`
  - `mosquitto_1_2.conf`
  - `mosquitto_3_4.conf`
5. The script generates a Mosquitto `passwd` file in `output/` using a pure Python implementation (PBKDF2-HMAC-SHA512):
  - `passwd`
6. The script generates an archive in `output/` containing generated certificates/keys, Mosquitto configurations, and `passwd`:
  - `mqtt_artifacts_<label>.zip`

Optional arguments:

- `--label <label>`: choose a custom label for the generated files (default is "mqtt").
- `--cert <type>`: specify the type of certificate to generate (`all`, `ca`, `client`, or `server`; default is `all`).
- `--nokey`: do not renew the key files for certificates (default is `False`).
- `--output <certs_dir>`: choose the output directory for the generated certificates and keys (default is `Certificates/lfs`).
- `--header-output <header_dir>`: choose the output directory for the generated C header file (default is `output/`).
- `--mqtt-username <username>`: choose the username used for generated Mosquitto `passwd` and C defines.
- `--mqtt-password <password>`: choose the password used for generated Mosquitto `passwd` and C defines.

The generated certificates and keys are defined as follows:

- CA Certificate: `ca_<label>.crt`
- Client Certificate: `client_<label>.crt`
- Client Private Key: `client_<label>.key`
- Server Certificate: `server_<label>.crt`
- Server Private Key: `server_<label>.key`

### __License__

See [LICENSE.md](LICENSE.md) for license details.
