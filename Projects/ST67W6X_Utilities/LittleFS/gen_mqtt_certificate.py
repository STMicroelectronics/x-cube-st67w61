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
import os
import subprocess
import argparse

# =====================================================
# Global variables
# =====================================================
USER_LABEL = "example"


# =====================================================
def run_openssl(command):
    try:
        subprocess.run(command, check=True, shell=True)
    except subprocess.CalledProcessError as e:
        print(f"An error occurred: {e}")


# =====================================================
# Main method
# =====================================================
if __name__ == '__main__':
    argparse.ArgumentParser(description="Generate MQTT certificates")
    parser = argparse.ArgumentParser(description="Generate MQTT certificates")
    parser.add_argument("--label", type=str, default=USER_LABEL, help="User label for certificate generation")
    parser.add_argument("--output", type=str, help="Output directory for generated certificates")
    args = parser.parse_args()

    if not args.output:
        args.output = os.path.join(os.path.dirname(os.path.realpath(__file__)), "Certificates", "lfs")

    if not os.path.exists(args.output):
        os.makedirs(args.output)

    USER_LABEL = args.label
    CA_KEY = os.path.join(args.output, f"ca_{USER_LABEL}.key")
    CA_CERT = os.path.join(args.output, f"ca_{USER_LABEL}.crt")
    CLIENT_KEY = os.path.join(args.output, f"client_{USER_LABEL}.key")
    CLIENT_CSR = os.path.join(args.output, f"client_{USER_LABEL}.csr")
    CLIENT_CERT = os.path.join(args.output, f"client_{USER_LABEL}.crt")
    SERVER_KEY = os.path.join(args.output, f"server_{USER_LABEL}.key")
    SERVER_CSR = os.path.join(args.output, f"server_{USER_LABEL}.csr")
    SERVER_CERT = os.path.join(args.output, f"server_{USER_LABEL}.crt")
    CLIENT_NAME = "mqtt_client"
    SERVER_NAME = "mqtt_server"

    # Generate a random CA RSA key
    run_openssl(f"openssl genrsa -out {CA_KEY} 2048")

    # Generate CA certificate
    run_openssl(f"openssl req -x509 -new -nodes -key {CA_KEY} -sha256 -days 11000 -out {CA_CERT} -subj \"/CN=MQTT CA\"")

    # Generate client private key
    run_openssl(f"openssl genrsa -out {CLIENT_KEY} 2048")

    # Generate client certificate request
    run_openssl(f"openssl req -new -key {CLIENT_KEY} -out {CLIENT_CSR} -subj \"/CN={CLIENT_NAME}\"")

    # Generate client certificate signed by CA
    run_openssl(f"openssl x509 -req -in {CLIENT_CSR} -CA {CA_CERT} -CAkey {CA_KEY} -CAcreateserial -out {CLIENT_CERT} -days 11000 -sha256")

    # Generate server private key
    run_openssl(f"openssl genrsa -out {SERVER_KEY} 2048")

    # Generate server certificate request
    run_openssl(f"openssl req -new -key {SERVER_KEY} -out {SERVER_CSR} -subj \"/CN={SERVER_NAME}\"")

    # Generate server certificate signed by CA
    run_openssl(f"openssl x509 -req -in {SERVER_CSR} -CA {CA_CERT} -CAkey {CA_KEY} -CAcreateserial -out {SERVER_CERT} -days 11000 -sha256")

    # Display generated files
    print("Generated files:")
    print(f"CA Key: {CA_KEY}")
    print(f"CA Certificate: {CA_CERT}")
    print(f"Client Key: {CLIENT_KEY}")
    print(f"Client Certificate: {CLIENT_CERT}")
    print(f"Server Key: {SERVER_KEY}")
    print(f"Server Certificate: {SERVER_CERT}")

    # Remove intermediates CSR files
    if os.path.exists(CLIENT_CSR):
        os.remove(CLIENT_CSR)
    if os.path.exists(SERVER_CSR):
        os.remove(SERVER_CSR)
    if os.path.exists(os.path.join(args.output, f"ca_{USER_LABEL}.srl")):
        os.remove(os.path.join(args.output, f"ca_{USER_LABEL}.srl"))
