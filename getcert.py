import requests
import os

# === Config ===
CERT_URL = "https://i.pki.goog/r1.pem"  # URL for Google Root CA in PEM format
OUTPUT_HEADER = "cert/google_root_ca.h"  # Path to the header file in the user's project
VAR_NAME = "google_root_ca"  # Variable name in the header file

def download_pem_cert():
    print(f"📥 Downloading GTS Root R1 PEM from {CERT_URL} ...")
    response = requests.get(CERT_URL)
    response.raise_for_status()
    return response.text.strip()

def pem_to_raw_string_literal(pem_str, var_name):
    return f"""\
#ifndef EMBER_GOOGLE_ROOT_CA_H
#define EMBER_GOOGLE_ROOT_CA_H

#ifdef ESP8266

const char {var_name}[] PROGMEM = R"CERT(
{pem_str}
)CERT";

#endif
#endif
"""

def write_header_file(content, path):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        f.write(content)
    print(f"📁 Header saved to: {path}")

def main():
    # Only regenerate if the header doesn't exist
    if os.path.exists(OUTPUT_HEADER):
        print(f"✅ Header already exists: {OUTPUT_HEADER} (skipping generation)")
        return

    print(f"⚙️  Header not found. Generating: {OUTPUT_HEADER}")
    pem_cert = download_pem_cert()
    header_content = pem_to_raw_string_literal(pem_cert, VAR_NAME)
    write_header_file(header_content, OUTPUT_HEADER)

if __name__ == "__main__":
    main()