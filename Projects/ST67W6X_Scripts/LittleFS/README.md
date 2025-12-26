## __Certificate and Little File System description__

Scripts to generate certificates and keys and to convert them into a LittleFS binary image.

> __IMPORTANT__
>
> The `littlefs/littlefs.bin` binary is required to load the NCP firmware on the ST67W611M Network Coprocessor.

### __Contents__

- [build.bat](#buildbat): Script to generate the LittleFS binary image containing certificates and keys.
- [gen_mqtt_certificate.py](#gen_mqtt_certificatepy): Python script to generate self-signed MQTT certificates and private keys for testing.

---

### __build.bat__ {#buildbat}

The NCP uses littlefs to read certificates and keys from the host processor, which are then store internally.

1. Place your certificates and keys in the `Certificates/lfs` folder.
2. Run the **`build.bat`** script to generate the `littlefs/littlefs.bin` file. This binary file contains the LittleFS image with your certificates and keys.

> **_NOTE:_** The `littlefs.bin` file is used during the NCP firmware flashing process described in the `../Binaries/README.md`.

---

### __gen_mqtt_certificate.py__ {#gen_mqtt_certificatepy}

This Python script generates a self-signed MQTT certificate and private key pair for testing purposes.

1. Ensure you have Python installed on your system.
2. Run the **`gen_mqtt_certificate.py`** script. This will create a self-signed certificate and private key in the `Certificates/lfs` folder.

The generated certificates and keys are defined as follows:

- CA Certificate: `ca_example.crt`
- Client Certificate: `client_example.crt`
- Client Private Key: `client_example.key`
- Server Certificate: `server_example.crt`
- Server Private Key: `server_example.key`

### License

See [LICENSE.md](LICENSE.md) for license details.
