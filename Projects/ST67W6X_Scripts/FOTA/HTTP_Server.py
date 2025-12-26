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
import itertools
import logging
import mimetypes
import threading
import traceback
import os
import http.server
import socketserver
import sys
import argparse
import ssl
import warnings
import signal
from concurrent.futures import ThreadPoolExecutor
from queue import Queue

# =====================================================
# Global variables
# =====================================================
# Configuration

# IP to use for the server
# if empty it bind itself to all interfaces available else it will bind to the IP specified
IP = ''

# Port to use for the server
PORT = 8000

HTTPS_PORT = 8443

# Directory where the firmware files are stored, those will be exposed by the server
FIRMWARE_DIR = "../Binaries/NCP_Binaries"

# Paths exposed by the server for the firmware files (ex doing GET /download/firmware.bin will return the file located at FIRMWARE_DIR/firmware.bin)
HTTP_FIRMWARE_DIR_PATH = "/download/"

# Main html page of the server
INDEX_PAGE = "/"

# Path exposed by the server to list all the firmware files available
BINARIES = "/binaries"

HTTP_DEFAULT_VERSION = "HTTP/1.1"

# Extension of the files to expose by the server
FILE_EXTENSIONS_TO_SHOW = [".bin", ".ota", ".xz", ".json"]

# logging level
DEFAULT_LOGGING_LEVEL = "DEBUG"

# Multi-threading option for the server itself to handle multiple HTTP(s) request
DEFAULT_THREADING_OPTION = True

# Default SSL certificate file
DEFAULT_SSL_CERTFILE = "cert.pem"

# Default SSL key file
DEFAULT_SSL_KEYFILE = "key.pem"

# Queue to keep track of server instances
server_queue = Queue()
# Create an object to manage event flags for signaling between threads
stop_event = threading.Event()

# =====================================================
# Try to import ThreadingMixIn for server threading
try:
    from socketserver import ThreadingMixIn

    class ThreadingHTTPServer(ThreadingMixIn, http.server.HTTPServer):
        """Handle requests in a separate thread."""

except ImportError:
    warnings.warn('Unable to import ThreadingMixIn. Falling back to single-threaded server.', ImportWarning)

    # Placeholder class for ThreadingMixIn to maintain compatibility when the import fails.
    class ThreadingMixIn:
        pass
    ThreadingHTTPServer = socketserver.TCPServer


# Create a local logger for this module
logger = logging.getLogger(__name__)


# =====================================================
# Configure logging level
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
# Generate a HTML page listing all files in the specified directory and its subdirectories up to a specified depth
def generate_binary_file_list_page(bin_dir: str, http_firmware_dir_path: str, file_extension_to_show: list[str], max_depth=1) -> str:
    """Generate an HTML page listing all binary files in the specified directory and its subdirectories up to a specified depth."""
    html_content = "<html><head><title>Available Binaries</title></head><body>"
    html_content += "<h1>Available Binaries</h1><ul>"

    for root, dirs, files in os.walk(bin_dir):
        # Calculate the current depth
        current_depth = root[len(bin_dir):].count(os.sep)
        if current_depth >= max_depth:
            # If the current depth is greater than or equal to max_depth, do not descend further
            dirs[:] = []

        for file in files:
            if any(file.endswith(suffix) for suffix in file_extension_to_show):
                relative_path = os.path.relpath(os.path.join(root, file), bin_dir)
                file_path = os.path.join(http_firmware_dir_path, relative_path)
                html_content += f'<li><a href="{file_path}">{relative_path}</a></li>'

    html_content += "</ul></body></html>"
    return html_content


# =====================================================
# Create a handler class to serve firmware files and handle update requests
class FOTARequestHandler(http.server.BaseHTTPRequestHandler):
    firmware_dir = FIRMWARE_DIR

    # https://en.wikipedia.org/wiki/List_of_Unicode_characters#Control_codes
    # Control characters table from the original HTTP server implementation
    # Used to replace Unicode Control character with escaped hex
    _control_char_table = str.maketrans(
        {c: fr'\x{c:02x}' for c in itertools.chain(range(0x20), range(0x7f, 0xa0))})
    _control_char_table[ord('\\')] = r'\\'

    # =================================================
    # Override HTTP server module logging message to redirect it to the local logger
    def log_message(self, format, *args):
        message = format % args
        logger.info("%s - - [%s] %s" %
                    (self.address_string(),
                     self.log_date_time_string(),
                     message.translate(self._control_char_table)))

    # =================================================
    # Override send_response to log HTTP header that will be sent to the HTTP client
    def send_header(self, keyword, value):
        logger.debug(f"Sending header: {keyword}: {value}")
        super().send_header(keyword, value)

    # =================================================
    # Override end_headers to log when the HTTP header end is reached
    def end_headers(self):
        logger.debug("End of headers\n")
        super().end_headers()

    # =================================================
    def set_firmware_dir(self, firmware_dir: str):
        """Set the firmware directory."""
        self.firmware_dir = firmware_dir

    # =================================================
    def do_GET(self):
        """Handle GET requests."""
        logger.debug(f"Received {self.command} request for {self.path}")
        logger.debug(f"GET request headers:\n{self.headers}")
        try:
            if self.path == INDEX_PAGE:
                self.serve_index_page()
            elif self.path == BINARIES:
                self.serve_file_list_page()
            elif self.path.startswith(HTTP_FIRMWARE_DIR_PATH):
                self.handle_download()
            elif self.path == "/favicon.ico":
                self.handle_favicon()
            else:
                self.send_error(404, "File not found")
        except ConnectionResetError as e:
            logger.warning("Connection reset by peer during GET request")
            self.log_exception(e)
        except Exception as e:
            self.log_exception(e)

    # =================================================
    def do_HEAD(self):
        """Handle HEAD requests."""
        logger.debug(f"Received {self.command} request for {self.path}")
        logger.debug(f"HEAD request headers:\n{self.headers}")
        try:
            if self.path == INDEX_PAGE:
                self.serve_index_page(head_only=True)
            elif self.path == BINARIES:
                self.serve_file_list_page(head_only=True)
            elif self.path.startswith(HTTP_FIRMWARE_DIR_PATH):
                self.handle_download(head_only=True)
            elif self.path == "/favicon.ico":
                self.handle_favicon(head_only=True)
            else:
                self.send_error(404, "File not found")
        except ConnectionResetError as e:
            logger.warning("Connection reset by peer during HEAD request")
            self.log_exception(e)
        except Exception as e:
            self.log_exception(e)

    # =================================================
    def do_POST(self):
        """Handle POST requests."""
        logger.debug(f"Received {self.command} request for {self.path}")
        logger.debug(f"POST request headers:\n{self.headers}")
        try:
            content_length = int(self.headers['Content-Length'])
            post_data = self.rfile.read(content_length)
            # Process the POST data here
            logger.info(f"Received POST data: {post_data.decode('utf-8')}")

            #  send a 303 redirect to the index page
            self.send_response(303)
            self.send_header("Location", "/")
            self.end_headers()
        except ConnectionResetError as e:
            logger.warning("Connection reset by peer during POST request")
            self.log_exception(e)
        except Exception as e:
            self.log_exception(e)

    # =================================================
    # Function to send the index page
    def serve_index_page(self, head_only=False):
        index_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'Resources', 'index.html')
        try:
            with open(index_path, 'r') as file:
                content = file.read()
            self.send_response(200)
            self.send_header("Content-Type", "text/html")
            self.send_header("Content-Length", str(len(content)))
            self.end_headers()
            if not head_only:
                self.wfile.write(content.encode())
        except FileNotFoundError:
            self.send_error(404, "File not found")
        except Exception as e:
            self.log_exception(e)

    # =================================================
    # Function to send the file list page
    def serve_file_list_page(self, head_only=False):
        try:
            html_content = generate_binary_file_list_page(self.firmware_dir, HTTP_FIRMWARE_DIR_PATH, FILE_EXTENSIONS_TO_SHOW, 6)
            self.send_response(200)
            self.send_header("Content-Type", "text/html")
            self.send_header("Content-Length", str(len(html_content)))
            self.end_headers()
            if not head_only:
                self.wfile.write(html_content.encode())
        except Exception as e:
            self.send_error(500, "Internal Server Error")
            self.log_exception(e)

    # =================================================
    # Support for favicon.ico used by web browsers
    def handle_favicon(self, head_only=False):
        favicon_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'Resources', 'favicon.ico')
        if os.path.exists(favicon_path):
            file_size = os.path.getsize(favicon_path)
            self.send_response(200)
            self.send_header("Content-Length", str(file_size))
            self.send_header("Content-Type", "image/x-icon")
            self.end_headers()

            if not head_only:
                with open(favicon_path, "rb") as favicon_file:
                    self.wfile.write(favicon_file.read())
                    self.wfile.flush()  # Ensure all data is sent
        else:
            self.send_error(404, "Favicon not found")

    # =================================================
    # Handles file downloads with support of content-range requests
    def handle_download(self, head_only=False):
        firmware_file = self.path[len(HTTP_FIRMWARE_DIR_PATH):]
        firmware_file_path = os.path.join(self.firmware_dir, firmware_file)
        if os.path.exists(firmware_file_path):
            file_size = os.path.getsize(firmware_file_path)
            range_header = self.headers.get('Range', None)
            if range_header:
                byte_range = range_header.replace('bytes=', '').split('-')
                start = int(byte_range[0])
                end = int(byte_range[1]) if byte_range[1] else file_size - 1
                length = end - start + 1

                self.send_response(206)
                self.send_header("Content-Range", f"bytes {start}-{end}/{file_size}")
                self.send_header("Content-Length", str(length))
                # Guess the MIME type based on the file extension
                mime_type, _ = mimetypes.guess_type(firmware_file_path)
                if not mime_type:
                    mime_type = "application/octet-stream"
                self.send_header("Content-Type", mime_type)
                self.send_header("Accept-Ranges", "bytes")
                self.end_headers()

                if not head_only:
                    with open(firmware_file_path, "rb") as file:
                        file.seek(start)
                        self.wfile.write(file.read(length))
                        self.wfile.flush()  # Ensure all data is sent
            else:
                self.send_response(200)
                self.send_header("Content-Length", str(file_size))
                # Guess the MIME type based on the file extension
                mime_type, _ = mimetypes.guess_type(firmware_file_path)
                if not mime_type:
                    mime_type = "application/octet-stream"
                self.send_header("Content-Type", mime_type)
                self.send_header("Accept-Ranges", "bytes")
                self.end_headers()

                if not head_only:
                    with open(firmware_file_path, "rb") as file:
                        self.wfile.write(file.read())
                        self.wfile.flush()  # Ensure all data is sent
        else:
            self.send_error(404, "Firmware file not found")

    # =================================================
    # Exception logging
    def log_exception(self, e):
        logger.error(f"Exception type: {type(e).__name__}")
        logger.error(f"Exception message: {str(e)}")
        logger.debug("Stack trace:")
        logger.debug(traceback.format_exc())
        logger.debug(f"Request method: {self.command}")
        logger.debug(f"Request path: {self.path}")
        logger.debug(f"Request headers: {self.headers}")
        logger.debug(f"Client address: {self.client_address}")
        logger.debug(f"Timestamp: {self.log_date_time_string()}")


# =====================================================
# Log server information
def print_info(httpd, http_request_handler):
    logger.debug(f"FOTA updates directory exposed: {http_request_handler.firmware_dir}")
    logger.debug(f"HTTP version: {http_request_handler.protocol_version}")
    logger.debug(f"Server version: {http_request_handler.server_version}")
    logger.debug(f"System version: {sys.version}")
    logger.debug(f"Server address: {httpd.server_address}")
    logger.debug(f"Server port: {httpd.server_port}")
    logger.debug(f"Server socket: {httpd.socket}")
    logger.debug(f"Server threading: {httpd.daemon_threads}")
    logger.debug(f"Server timeout: {httpd.timeout}")
    logger.debug(f"Server request handler: {httpd.RequestHandlerClass}")
    logger.debug(f"Server request handler firmware directory path: {HTTP_FIRMWARE_DIR_PATH}")
    logger.debug(f"Server request handler file extensions to show: {FILE_EXTENSIONS_TO_SHOW}")


# =====================================================
# Function to start the server and handle the start in a new thread
def handle_http_start_server(httpd, executor):
    logger.info("Starting server on %s:%s\n", *httpd.server_address)

    def run_server():
        try:
            # Start the HTTP server
            httpd.serve_forever()
        except KeyboardInterrupt:
            pass
        finally:
            # Shutdown the server
            httpd.server_close()
            logger.debug("Server stopped.")

    # Add the server to the queue
    server_queue.put(httpd)

    # Start the server in a new thread using ThreadPoolExecutor
    executor.submit(run_server)


# =====================================================
# Function to stop all servers
def stop_all_servers():
    logger.info("Stopping all servers...")
    while not server_queue.empty():
        httpd = server_queue.get()
        httpd.shutdown()
    logger.info("All servers stopped.")


# =====================================================
# Signal handler to stop all servers
def signal_handler(sig, frame):
    logger.debug("Signal %s received, stopping servers.", sig)
    stop_all_servers()
    stop_event.set()


# =====================================================
# Main function
# =====================================================
# Functions that create the HTTP server and start it, can be used with or without threading, with or without arg parser
def parse_args():
    parser = argparse.ArgumentParser(description='Start the FOTA HTTP server.')
    # Global arguments
    parser.add_argument('-l', '--log-level', type=str, default=DEFAULT_LOGGING_LEVEL, help='Logging level (e.g., DEBUG, INFO, WARNING, ERROR, CRITICAL)')
    parser.add_argument('-f', '--firmware-dir', type=str, default='', help='Directory where the firmware files are stored')
    parser.add_argument('-t', '--threaded', default=DEFAULT_THREADING_OPTION, action='store_true', help='Run the server in threaded mode')

    # HTTP Server arguments
    parser.add_argument('-p', '--port', type=int, default=PORT, help='Port to run the server on')
    parser.add_argument('-i', '--ip', type=str, default=IP, help='IP address to run the server on')
    parser.add_argument('-hv', '--http-version', type=str, default=HTTP_DEFAULT_VERSION, help='HTTP version to use (e.g., HTTP/1.1, HTTP/2)')

    # HTTPS Server arguments
    parser.add_argument('-s', '--enable_https', action='store_true', help='Enable or disable HTTPS server')
    parser.add_argument('-ps', '--https-port', type=int, default=HTTPS_PORT, help='Port to run the HTTPS server on')
    parser.add_argument('-c', '--certfile', type=str, default=DEFAULT_SSL_CERTFILE, help='Path to the SSL certificate file')
    parser.add_argument('-k', '--keyfile', type=str, default=DEFAULT_SSL_KEYFILE, help='Path to the SSL key file')

    args = parser.parse_args()

    if not args.firmware_dir:
        args.firmware_dir = os.path.join(os.path.dirname(os.path.realpath(__file__)), FIRMWARE_DIR)
    elif not os.path.exists(args.firmware_dir):
        raise FileNotFoundError(f'Firmware directory not found: {args.firmware_dir}')

    return args


# =====================================================
# Check Python version compatibility
def compatibility_check():
    logger.debug(
        "This script doesn't require external dependencies outside of Python builtins and the standard library.")
    # Check Python version
    if sys.version_info[0] < 3:
        raise Exception('Must be using Python 3')
    if sys.version_info[1] != 11:
        warnings.warn('This script was written with Python 3.11 in mind', DeprecationWarning)


# =====================================================
# Create an SSL context with the given certificate and key files
def create_ssl_context(certfile, keyfile):
    """Create an SSL context with the given certificate and key files."""
    context = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
    context.load_cert_chain(certfile=certfile, keyfile=keyfile)
    return context


# =====================================================
# Create an HTTP server with the given IP, port, HTTP version, threading option, firmware directory, and SSL context
def create_server(ip, port, http_version, threaded, firmware_dir, ssl_context=None):
    FOTARequestHandler.protocol_version = http_version
    FOTARequestHandler.firmware_dir = firmware_dir
    server_class = ThreadingHTTPServer if threaded else socketserver.TCPServer
    httpd = server_class((ip, port), FOTARequestHandler)
    if ssl_context:
        httpd.socket = ssl_context.wrap_socket(httpd.socket, server_side=True)
    return httpd


# =====================================================
# Function to start the server with arg parser
def start_server():
    compatibility_check()
    args = parse_args()
    # Build a unique log file name using subparser and timestamp
    now = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    log_file_name = f"log_http_server_{now}.txt"
    log_file_path = os.path.join(os.getcwd(), log_file_name)

    configure_local_logging(getattr(args, "log_level", DEFAULT_LOGGING_LEVEL), log_file_path)

    # Register signal handlers for SIGINT and SIGTERM
    # SIGINT is sent by the user pressing Ctrl+C
    # SIGTERM is sent by the system when the user sends a termination signal (closing the window of running program for example)
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    # Create a ThreadPoolExecutor to manage server threads
    executor = ThreadPoolExecutor(max_workers=5)

    # Start HTTP server
    httpd = create_server(args.ip, args.port, args.http_version, args.threaded, args.firmware_dir)
    print_info(httpd, FOTARequestHandler)
    handle_http_start_server(httpd, executor)

    if args.enable_https:
        # Start HTTPS server
        ssl_context = create_ssl_context(args.certfile, args.keyfile)
        httpsd = create_server(args.ip, args.https_port, args.http_version, args.threaded, args.firmware_dir, ssl_context)
        print_info(httpsd, FOTARequestHandler)
        handle_http_start_server(httpsd, executor)

    # Wait for the stop event to be set, else keep running
    while not stop_event.is_set():
        stop_event.wait(timeout=1)

    # Shutdown the executor
    executor.shutdown(wait=True)


# =====================================================
# Main function if the script is run directly without importing
# =====================================================
if __name__ == '__main__':
    start_server()
