// SPDX-License-Identifier: MIT
// Copyright (C) 2024-2025, Advanced Micro Devices, Inc. All rights reserved.

#ifndef _AIEBU_COMMON_WRITER_H_
#define _AIEBU_COMMON_WRITER_H_

#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include "symbol.h"
#include "code_section.h"

namespace aiebu {

// Class to hold sections name, data, symbols, type
class writer
{
public:
  virtual ~writer() = default;
};

class section_writer: public writer
{
  const std::string m_name;
  const code_section m_type;
  std::vector<uint8_t> m_data;
  std::vector<symbol> m_symbols;
  std::unordered_map<std::string, std::string> m_metadata;

public:
  section_writer(std::string name, code_section type, std::vector<uint8_t>&& data)
    : m_name(std::move(name)),
      m_type(type),
      m_data(std::move(data)) {}
  section_writer(std::string name, code_section type): m_name(std::move(name)), m_type(type) {}
  virtual ~section_writer() = default;

  virtual void write_byte(uint8_t byte);

  virtual void write_word(uint32_t word);

  virtual uint32_t read_word(offset_type offset) const;

  virtual void write_word_at(offset_type offset, uint32_t word);

  virtual offset_type tell() const;

  const std::vector<uint8_t>&
  get_data() const
  {
    return m_data;
  }

  const std::string&
  get_name() const
  {
    return m_name;
  }

  code_section
  get_type() const
  {
    return m_type;
  }

  void set_data(std::vector<uint8_t> &data)
  {
    m_data = std::move(data);
  }

  const std::vector<symbol>&
  get_symbols() const
  {
    return m_symbols;
  }

  void add_symbol(symbol sym)
  {
    m_symbols.emplace_back(sym);
  }

  void add_symbols(std::vector<symbol>& syms)
  {
    m_symbols = std::move(syms);
  }

  bool hassymbols() const
  {
    return m_symbols.size();
  }

  void padding(offset_type size);

  void add_metadata(std::unordered_map<std::string, std::string>&& metadata)
  {
    m_metadata = std::move(metadata);
  }

  std::string&
  get_metadata(const std::string& key)
  {
    return m_metadata[key];
  }
};

class config_writer: public writer
{
  std::map<std::string, std::map<std::string, std::vector<std::shared_ptr<writer>>>> m_output;
  partition_info m_partition;

public:
  const std::map<std::string, std::map<std::string,  std::vector<std::shared_ptr<writer>>>>&
  get_kernel_map() const { return m_output; }

  void add_kernel_map(const std::string& kernel, const std::string& instance, std::vector<std::shared_ptr<writer>> val) {
    m_output[kernel][instance] = std::move(val);
  }

  uint32_t get_numcore() const { return m_partition.core; }

  uint32_t get_numcolumn() const { return m_partition.column; }

  uint32_t get_nummem() const { return m_partition.mem; }

  void set_numcolumn(uint32_t val) { m_partition.column = val; }

  void set_numcore(uint32_t val) { m_partition.core = val; }

  void set_nummem(uint32_t val) { m_partition.mem = val; }
};

}
#endif //_AIEBU_COMMON_WRITER_H_
