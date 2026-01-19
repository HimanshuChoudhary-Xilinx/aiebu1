#!/usr/bin/env python3
"""
Generate AIE4 save/restore header file from assembly source files.
Converts entire .asm file content to byte arrays for embedding in C++ header.
"""

import os
import sys

def read_file_as_bytes(filepath, label_prefix=None):
    """
    Read entire file content and return as list of bytes.
    Optionally prepend a label (e.g., "save:" or "restore:") and append .endl directive.
    """
    bytes_list = []

    if not os.path.exists(filepath):
        print(f"Warning: File not found: {filepath}")
        return bytes_list

    with open(filepath, 'rb') as f:
        content = f.read()

    # Prepend label and append .endl if label_prefix specified
    if label_prefix:
        # Remove trailing colon for .endl directive (e.g., "save:" -> "save")
        label_name = label_prefix.rstrip(':')
        label_bytes = (label_prefix + "\n").encode('utf-8')
        endl_bytes = ("\n.endl " + label_name + "\n").encode('utf-8')
        content = label_bytes + content + endl_bytes

    bytes_list = list(content)

    return bytes_list

def format_bytes_array(bytes_list):
    """Format a list of bytes as a C++ initializer list."""
    if not bytes_list:
        return "{}"

    hex_bytes = [f"0x{b:02x}" for b in bytes_list]
    return "{" + ", ".join(hex_bytes) + "}"

def generate_header(asm_dir, output_file):
    """Generate the header file from assembly files."""

    # Define the mapping: key -> (save_file, restore_file)
    file_mappings = {
        1: ("aie4_save_1c.asm", "aie4_restore_1c.asm"),
        2: ("aie4_save_2c.asm", "aie4_restore_2c.asm"),
        3: ("aie4_save_3c.asm", "aie4_restore_3c.asm"),
    }

    # Parse all files
    data = {}
    for key, (save_file, restore_file) in file_mappings.items():
        save_path = os.path.join(asm_dir, save_file)
        restore_path = os.path.join(asm_dir, restore_file)

        save_bytes = read_file_as_bytes(save_path, "save:")
        restore_bytes = read_file_as_bytes(restore_path, "restore:")

        if save_bytes or restore_bytes:
            data[key] = (save_bytes, restore_bytes)
            print(f"Key {key}: save={len(save_bytes)} bytes, restore={len(restore_bytes)} bytes")

    # Generate header content
    header = """// SPDX-License-Identifier: MIT
// Copyright (C) 2026, Advanced Micro Devices, Inc. All rights reserved.

#ifndef AIEBU_AIE4_PREEMPTION_FILES_H
#define AIEBU_AIE4_PREEMPTION_FILES_H

#include <map>
#include <vector>
#include <cstdint>

inline std::map<uint32_t, std::pair<std::vector<uint8_t>, std::vector<uint8_t>>>&
get_aie4_save_restore()
{
  static std::map<uint32_t, std::pair<std::vector<uint8_t>, std::vector<uint8_t>>> aie4_save_restore_map = {
"""

    # Add entries
    entries = []
    for key in sorted(data.keys()):
        save_bytes, restore_bytes = data[key]
        save_str = format_bytes_array(save_bytes)
        restore_str = format_bytes_array(restore_bytes)
        entries.append(f"    {{{key}, {{{save_str}, {restore_str}}}}}")

    header += ",\n".join(entries)

    header += """
  };
  return aie4_save_restore_map;
}

#endif // AIEBU_AIE4_PREEMPTION_FILES_H
"""

    # Write output file
    with open(output_file, 'w') as f:
        f.write(header)

    print(f"Generated: {output_file}")

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    # Assembly files are in the same directory as this script
    asm_dir = script_dir

    # Default output path is current working directory
    output_file = "aie4_save_restore_map_prebuilt.h"

    # Allow command line override
    if len(sys.argv) > 1:
        output_file = sys.argv[1]

    if len(sys.argv) > 2:
        asm_dir = sys.argv[2]

    generate_header(asm_dir, output_file)

if __name__ == "__main__":
    main()
