#!/usr/bin/python3.11
import re
import gzip
import os
import sys


def extract_compressed_data(header_file):
    """Extracts the compressed data array from camera_index.h."""
    with open(header_file, "r") as file:
        content = file.read()

    # Match the array definition in the header file
    match = re.search(r"const uint8_t .*?\[\] = \{(.+?)\};", content, re.DOTALL)
    if not match:
        raise ValueError("No valid array found in the provided header file.")

    # Extract and clean the byte data
    data = match.group(1).strip().replace("\n", "").split(",")
    return bytearray(int(byte.strip(), 0) for byte in data if byte.strip())


def extract_uncompressed_data(header_file):
    """Extracts the uncompressed HTML content from camera_index.h."""
    with open(header_file, "r") as file:
        content = file.read()

    # Match the uncompressed string definition
    match = re.search(r'const char .*?\[\] = R"(\((.*?)\));"', content, re.DOTALL)
    if not match:
        raise ValueError("No valid uncompressed HTML content found in the provided header file.")

    # Return the raw HTML content
    return match.group(2).encode("utf-8")


def decompress_data(data):
    """Decompresses the GZIP data."""
    return gzip.decompress(data)


def compress_data(data):
    """Compresses the HTML data using GZIP."""
    return gzip.compress(data, compresslevel=9)


def generate_uncompressed_header(decompressed_data, output_file):
    """Generates a new camera_index.h file with the decompressed HTML content."""
    html_content = decompressed_data.decode("utf-8")

    # Use raw string literal (R"()" syntax) for embedding HTML
    with open(output_file, "w") as file:
        file.write('const char camera_index_html[] = R"(\n')
        file.write(html_content)
        file.write('\n)";\n')
        file.write(f"const unsigned int camera_index_html_len = {len(decompressed_data)};\n")

    print(f"Uncompressed header file written to: {output_file}")


def generate_compressed_header(compressed_data, output_file):
    """Generates a new camera_index.h file with the compressed HTML content."""
    compressed_array = ", ".join(f"0x{byte:02x}" for byte in compressed_data)

    with open(output_file, "w") as file:
        file.write("const unsigned char camera_index_html_gz[] = {\n")
        for i in range(0, len(compressed_array), 12 * 5):  # Split into lines with up to 12 bytes
            file.write("    " + compressed_array[i:i + 12 * 5] + ",\n")
        file.write("};\n")
        file.write(f"const unsigned int camera_index_html_gz_len = {len(compressed_data)};\n")

    print(f"Compressed header file written to: {output_file}")


def print_help():
    """Prints a short help message."""
    print("Usage: python convert_camera_index_bidirectional.py [--decompress|--compress] <input_file> <output_file>")
    print("Options:")
    print("  --decompress  : Convert a compressed camera_index.h file to uncompressed.")
    print("  --compress    : Convert an uncompressed camera_index.h file to compressed.")
    print("Example:")
    print("  python convert_camera_index_bidirectional.py --decompress camera_index.h uncompressed_camera_index.h")
    print("  python convert_camera_index_bidirectional.py --compress uncompressed_camera_index.h camera_index.h")


def main():
    if len(sys.argv) != 4:
        print_help()
        return

    option = sys.argv[1]
    input_file = sys.argv[2]
    output_file = sys.argv[3]

    if not os.path.exists(input_file):
        print(f"Error: Input file '{input_file}' not found.")
        return

    try:
        if option == "--decompress":
            print(f"Decompressing '{input_file}'...")
            compressed_data = extract_compressed_data(input_file)
            decompressed_data = decompress_data(compressed_data)
            generate_uncompressed_header(decompressed_data, output_file)
        elif option == "--compress":
            print(f"Compressing '{input_file}'...")
            uncompressed_data = extract_uncompressed_data(input_file)
            compressed_data = compress_data(uncompressed_data)
            generate_compressed_header(compressed_data, output_file)
        else:
            print("Invalid option!")
            print_help()
    except Exception as e:
        print(f"Error: {e}")


if __name__ == "__main__":
    main()
