// SPDX-License-Identifier: MIT
// Copyright (C) 2025 Advanced Micro Devices, Inc. All rights reserved.

#ifndef AIEBU_TRANSFORM_MANAGER_H_
#define AIEBU_TRANSFORM_MANAGER_H_

#include <cstdint>

#include <elfio/elfio.hpp>
#include "specification/aie2ps/isa.h"
#include "common/symbol.h"

namespace aiebu {

class transform_manager {

  static constexpr size_t ELF_SECTION_HEADER_SIZE = 16;  // ELF-specific header padding
  static constexpr uint8_t ALIGN_OPCODE = 0xA5;  // .align pseudo-instruction opcode
  static constexpr size_t CTRLTEXT_STRING_LENGTH = 9;  // Length of ".ctrltext"
  static constexpr size_t CTRLDATA_STRING_LENGTH = 9;  // Length of ".ctrldata"
  static constexpr size_t CTRLPKT_STRING_LENGTH = 8;  // Length of ".ctrlpkt"
  static constexpr size_t CTRLCODE_STRING_LENGTH = 13;  // Length of "control-code"
  static constexpr uint8_t Elf_Amd_Aie2ps       = 64;
  static constexpr uint8_t Elf_Amd_Aie2ps_group = 70;
  static constexpr uint8_t Num_32bit_Register = 2;

  ELFIO::elfio m_elfio;
  std::map<std::string, uint32_t> xrt_idx_lookup;
  const std::map<uint8_t, isa_op_disasm>* isa_op_map;
  isa_disassembler m_isa_disassembler;

  std::string get_key(uint32_t offset, uint32_t section_idx)
  {
    // Lookup key as offset and section idx combination will be always unique
    return std::to_string(offset) + "_" + std::to_string(section_idx);
  }

  uint32_t size(const isa_op_disasm op) const;
  void modify_apply_offset_57(char* text_section_data, size_t text_section_size, uint32_t section_idx);
  void process_sections();
  std::pair<uint32_t, uint32_t> get_column_and_page(const std::string& name) const;
  uint64_t read57(const uint32_t* bd_data_ptr) const;
  uint64_t read57_aie4(const uint32_t* bd_data_ptr) const;
  void write57(uint32_t* bd_data_ptr, uint64_t bd_offset);
  void write57_aie4(uint32_t* bd_data_ptr, uint64_t bd_offset);
  uint64_t ctrlpkt_read57(const uint32_t* bd_data_ptr) const;
  void ctrlpkt_write57(uint32_t* bd_data_ptr, uint64_t bd_offset);
  uint64_t ctrlpkt_read57_aie4(const uint32_t* bd_data_ptr) const;
  void ctrlpkt_write57_aie4(uint32_t* bd_data_ptr, uint64_t bd_offset);
  uint64_t get_controlcode_bd_offset(uint32_t col, uint32_t page, uint32_t offset, symbol::patch_schema schema);
  void set_controlcode_bd_offset(uint32_t col, uint32_t page, uint32_t offset, uint64_t bd_offset, symbol::patch_schema schema);
  uint64_t get_ctrlpkt_bd_offset(std::string& section_name, uint32_t offset, symbol::patch_schema schema);
  void set_ctrlpkt_bd_offset(std::string& section_name, uint32_t offset, uint64_t bd_offset, symbol::patch_schema schema);

  static std::string get_ctrltext_section_name(uint32_t col, uint32_t page)
  {
    return ".ctrltext." + std::to_string(col) + "." + std::to_string(page);
  }

  static std::string get_ctrldata_section_name(uint32_t col, uint32_t page)
  {
    return ".ctrldata." + std::to_string(col) + "." + std::to_string(page);
  }

  static bool is_text_section(const std::string& section_name)
  {
    return !section_name.substr(0,CTRLTEXT_STRING_LENGTH).compare(".ctrltext");
  }

  static bool is_text_or_data_section_name(const std::string& str)
  {
    return !str.substr(0,CTRLTEXT_STRING_LENGTH).compare(".ctrltext") || !str.substr(0,CTRLDATA_STRING_LENGTH).compare(".ctrldata");
  }

  static bool is_ctrlpkt_section_name(const std::string& str)
  {
    return !str.substr(0,CTRLPKT_STRING_LENGTH).compare(".ctrlpkt");
  }

  static bool is_ctrlpkt_patch_name(const std::string& str)
  {
    return !str.substr(0,CTRLPKT_STRING_LENGTH).compare(".ctrlpkt");
  }

  static bool is_controlcode_patch_name(const std::string& str)
  {
    return !str.substr(0,CTRLCODE_STRING_LENGTH).compare("control-code");
  }

public:
  explicit transform_manager(const std::vector<char>& elf_data);

  void load_elf(const std::vector<char>& elf_data);
  ~transform_manager() = default;

  transform_manager(const transform_manager &) = delete;
  transform_manager(transform_manager &&) = delete;
  transform_manager &operator=(const transform_manager &) = delete;
  transform_manager& operator=(transform_manager &&) = delete;

  std::vector<arginfo> extract_rela_sections();
  std::vector<char> update_rela_sections(const std::vector<arginfo>& entries);
};

}

#endif
