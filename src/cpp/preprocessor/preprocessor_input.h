// SPDX-License-Identifier: MIT
// Copyright (C) 2024-2025, Advanced Micro Devices, Inc. All rights reserved.
#ifndef AIEBU_PREPROCESSOR_PREPROCESSOR_INPUT_H_
#define AIEBU_PREPROCESSOR_PREPROCESSOR_INPUT_H_

#include "symbol.h"
#include "aiebu/aiebu_error.h"
#include "aiebu/aiebu_assembler.h"
#include "file_utils.h"

#include <boost/property_tree/ptree.hpp>

#include <algorithm>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <vector>
#include <unordered_map>
#include <string>

namespace aiebu {

class global_custom_section_storage
{
  std::map<std::string, std::vector<uint8_t>> m_map;

public:
  const std::map<std::string, std::vector<uint8_t>>& map() const { return m_map; }
  void assign(std::map<std::string, std::vector<uint8_t>> parsed) { m_map = std::move(parsed); }
};

class preprocessor_input
{
protected:
  std::map<std::string, std::vector<char>> m_data;
  std::map<std::string, std::vector<char>> m_controlpkt;
  std::vector<symbol> m_sym;

  class argument {
    public:
    std::string name;
    std::string type;
    std::string offset;

    argument(std::string na, std::string ty, std::string off):
             name(std::move(na)),
             type(std::move(ty)),
             offset(std::move(off)) {}
    argument(const argument& rhs) = default;
    argument& operator=(const argument& rhs) = default;
    argument(argument &&s) = default;
    ~argument() = default;
    argument& operator =(argument &&) = default;
  };

  struct function {
    std::string name;
    std::vector<argument> arguments;
  };

  std::string mangle_function_name(const function& func) {
    std::string mangled_name = "_Z" + std::to_string(func.name.length()) + func.name;
    for (const auto& arg : func.arguments) {
        if (arg.type == "char *") {
            mangled_name += "Pc"; // 'Pc' represents 'char *' in Itanium C++ ABI
        } else if (arg.type == "void *") {
            mangled_name += "Pv"; // 'Pv' represents 'void *' in Itanium C++ ABI
        } else if (arg.type == "scalar") {
            mangled_name += "i"; // 'i' represents 'int' in Itanium C++ ABI for scalar
        } else if (arg.type == "int *") {
            mangled_name += "Pi"; // 'Pi' represents 'int *' in Itanium C++ ABI
        }
    }
    return mangled_name;
  }

  std::map<std::string, std::vector<uint8_t>> parse_custom_sections(
      const boost::property_tree::ptree& pt,
      const std::vector<std::string>& paths,
      const file_artifact* artifacts = nullptr)
  {
    std::map<std::string, std::vector<uint8_t>> custom_sections;
    const auto& pt_custom_sections = pt.get_child_optional("custom_section");
    if (pt_custom_sections) {
      for (const auto& [sec_unused, section] : pt_custom_sections.get()) {
        auto section_name = section.get<std::string>("section_name");
        if (custom_sections.count(section_name))
          throw error(error::error_code::invalid_input,
                     "custom_section: duplicate section_name \"" + section_name + "\"");
        auto section_path = section.get<std::string>("path");
        std::vector<char> section_data;
        try {
          if (artifacts)
            section_data = artifacts->get(section_path, paths);
          else
            throw std::runtime_error("artifacts is null");
        } catch (const std::runtime_error& e) {
          throw error(error::error_code::invalid_input,
                     "custom_section: error reading buffer from artifacts for \"" + section_path + "\": " + e.what());
        }
        custom_sections[section_name] = std::vector<uint8_t>(section_data.begin(), section_data.end());
      }
    }
    return custom_sections;
  }

public:
  preprocessor_input() {}
  virtual ~preprocessor_input() = default;

  virtual void set_args(const std::vector<char>&,
                        const std::vector<char>& patch_json,
                        const std::vector<char>&,
                        const std::vector<std::string>&,
                        const std::vector<std::string>&,
                        const std::map<uint32_t, std::vector<char> >& ctrlpkt,
                        const file_artifact* /*resolver*/ = nullptr) = 0;

  const std::vector<std::string> get_keys()
  {
    std::vector<std::string> keys(m_data.size());
    auto key_selector = [](auto pair){return pair.first;};
    transform(m_data.begin(), m_data.end(), keys.begin(), key_selector);
    return keys;
  }

  virtual std::vector<char>& get_data(const std::string& key)
  {
    auto it = m_data.find(key);
    if (it == m_data.end())
      throw error(error::error_code::internal_error, "Key (" + key  + ") not found!!!");
    return m_data[key];
  }

  std::vector<symbol>& get_symbols()
  {
    return m_sym;
  }

  void add_symbol(const symbol buf)
  {
    m_sym.emplace_back(buf);
  }

  void add_symbols(const std::vector<symbol>& syms)
  {
    m_sym.insert( m_sym.end(), syms.begin(), syms.end());
  }
};

}
#endif // AIEBU_PREPROCESSOR_PREPROCESSOR_INPUT_H_
