// SPDX-License-Identifier: MIT
// Copyright (C) 2025, Advanced Micro Devices, Inc. All rights reserved.

#ifndef AIEBU_COMMOM_FILE_UTILS_H_
#define AIEBU_COMMOM_FILE_UTILS_H_

#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "aiebu/aiebu_error.h"
#include "aiebu/aiebu_assembler.h"

namespace aiebu {

// Defined in AIE2p Architecture Specification v1.4
struct cp_pktheader_aie2p
{
  uint32_t stream_packet_ID : 5;
  uint32_t out_of_order_bd_idx : 6;
  uint32_t reserved_a_0 : 1;
  uint32_t stream_id_rtn : 3;
  uint32_t reserved_b_0 : 1;
  uint32_t source_row : 5;
  uint32_t source_col : 7;
  uint32_t reserved_c_000 : 3;
  uint32_t odd_parity : 1;
};

// Defined in AIE2p Architecture Specification v1.4
struct cp_ctrlinfo_aie2p
{
  uint32_t local_byte_addr : 20;
  uint32_t num_data_beat : 2;
  uint32_t operation : 2;
  uint32_t stream_id_rtn : 5;
  uint32_t reserved_00 : 2;
  uint32_t parity : 1;
};

inline std::vector<char>
readfile(const std::string& filename)
{
  if (!std::filesystem::exists(filename))
    throw error(error::error_code::internal_error, "file:" + filename + " not found\n");

  std::ifstream input(filename, std::ios::in | std::ios::binary);
  auto file_size = std::filesystem::file_size(filename);

  if (!file_size)
    throw error(error::error_code::invalid_asm, "filename " + filename + " is empty!!");

  std::cout << "READING: " << filename <<"\n";
  std::vector<char> buffer(file_size);
  input.read(buffer.data(), static_cast<std::streamsize>(file_size));
  return buffer;
}

inline bool
isAbsolutePath(const std::string& path) {
  // On Unix-like systems, an absolute path starts with '/'
  if (path.empty()) {
    return false;
  }
  if (path[0] == '/') {
    return true;
  }
  // On Windows, an absolute path can start with a drive letter followed by ':'
  // and a backslash or forward slash, e.g., "C:\\" or "C:/"
  if (path.size() > 1 && path[1] == ':' && (path[2] == '\\' || path[2] == '/')) {
    return true;
  }
  return false;
}

inline std::vector<char>
readfile(const std::string& file, const std::vector<std::string>& paths)
{
  if (isAbsolutePath(file))
    return readfile(file);

  for (auto& path : paths)
  {
    std::string fullpath = path + "/" + file;
    if (std::filesystem::exists(fullpath))
      return readfile(fullpath);
  }

  throw error(error::error_code::internal_error, "File " + file + " not exist\n");
  return {};
}

inline std::string
findFilePath(const std::string& filename, const std::vector<std::string>& libpaths)
{

  if (isAbsolutePath(filename))
    return filename;
  for (const auto &dir : libpaths ) {
    auto ret = std::filesystem::exists(dir + "/" + filename);
    if (ret) {
      return dir + "/" + filename;
    }
  }
  throw error(error::error_code::internal_error, filename + " file not found!!\n");
}

aiebu_assembler::buffer_type
identify_buffer_type(const std::vector<char> &buffer);

aiebu_assembler::buffer_type
identify_control_packet(const char* buffer, uint64_t size);

inline std::string get_parent_directory(const std::string& relativePath) {
  std::filesystem::path absolutePath = std::filesystem::absolute(relativePath);  // Convert relative to absolute
  return absolutePath.parent_path().string();  // Return the parent directory
}

}

#endif // AIEBU_COMMOM_FILE_UTILS_H_
