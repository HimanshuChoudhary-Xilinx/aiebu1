// SPDX-License-Identifier: MIT
// Copyright (C) 2024, Advanced Micro Devices, Inc. All rights reserved.

#ifndef _AIEBU_PREPROCESSOR_PREPROCESSED_OUTPUT_H_
#define _AIEBU_PREPROCESSOR_PREPROCESSED_OUTPUT_H_

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace aiebu {

class preprocessed_output
{
public:
  preprocessed_output() = default;
  virtual ~preprocessed_output() = default;
};

class config_preprocessed_output_base : public preprocessed_output
{
  std::map<std::string, std::vector<uint8_t>> m_global_custom_sections;

public:
  void set_global_custom_sections(const std::map<std::string, std::vector<uint8_t>>& sections)
  {
    m_global_custom_sections = sections;
  }

  const std::map<std::string, std::vector<uint8_t>>& get_global_custom_sections() const
  {
    return m_global_custom_sections;
  }
};

template <typename OutPtr, typename InPtr>
void propagate_global_custom_sections(OutPtr out, InPtr in)
{
  out->set_global_custom_sections(in->get_global_custom_sections());
}

}

#endif //_AIEBU_PREPROCESSOR_PREPROCESSED_OUTPUT_H_
