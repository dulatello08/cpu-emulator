#!/usr/bin/env python3
"""
bin2hex.py - Convert binary file to hex format for Verilog $readmemh

Usage: python3 bin2hex.py input.bin output.hex
"""

import sys

def bin_to_hex(input_file, output_file):
    """Convert binary file to hex format, one byte per line."""
    try:
        with open(input_file, 'rb') as f:
            data = f.read()
        
        with open(output_file, 'w') as f:
            f.write("// Binary file: {}\n".format(input_file))
            f.write("// Size: {} bytes\n".format(len(data)))
            for byte in data:
                f.write("{:02X}\n".format(byte))
        
        print("Successfully converted {} to {}".format(input_file, output_file))
        print("  {} bytes written".format(len(data)))
        
    except FileNotFoundError:
        print("Error: File '{}' not found".format(input_file))
        sys.exit(1)
    except Exception as e:
        print("Error: {}".format(e))
        sys.exit(1)

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: {} <input.bin> <output.hex>".format(sys.argv[0]))
        sys.exit(1)
    
    bin_to_hex(sys.argv[1], sys.argv[2])
