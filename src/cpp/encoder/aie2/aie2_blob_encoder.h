// SPDX-License-Identifier: MIT
// Copyright (C) 2024-2025, Advanced Micro Devices, Inc. All rights reserved.

#ifndef _AIEBU_ENCODER_AIE2_BLOB_ENCODER_H_
#define _AIEBU_ENCODER_AIE2_BLOB_ENCODER_H_

#include <memory>

#include "encoder.h"
#include "aie2_blob_preprocessed_output.h"

namespace aiebu {

class aie2_blob_encoder: public encoder
{
public:
  aie2_blob_encoder() {}

  std::vector<std::shared_ptr<writer>> process(std::shared_ptr<preprocessed_output> input) override
  {
    // encode : nothing to be done as blob is already encoded
    auto rinput = std::static_pointer_cast<aie2_blob_preprocessed_output>(input);
    std::vector<std::shared_ptr<writer>> rwriter;

    for(auto key : rinput->get_keys())
      if ( !key.compare(".ctrltext") )
        rwriter.emplace_back(std::make_shared<section_writer>(key, code_section::text, std::move(rinput->get_data(key))));
      else
        rwriter.emplace_back(std::make_shared<section_writer>(key, code_section::data, std::move(rinput->get_data(key))));

    std::shared_ptr<section_writer> element = std::dynamic_pointer_cast<section_writer>(rwriter[0]);
    element->add_symbols(rinput->get_symbols());
    element->add_metadata(std::move(rinput->get_metadata()));
    return rwriter;
  }
};

}
#endif //_AIEBU_ENCODER_AIE2_BLOB_ENCODER_H_
