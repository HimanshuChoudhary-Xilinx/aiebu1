// SPDX-License-Identifier: MIT
// Copyright (C) 2024-2025, Advanced Micro Devices, Inc. All rights reserved.

#ifndef _AIEBU_ENCODER_AIE2PS_ENCODER_H_
#define _AIEBU_ENCODER_AIE2PS_ENCODER_H_

#include <memory>

#include "encoder.h"
#include "writer.h"
#include "aie2ps_preprocessed_output.h"
#include "ops.h"
#include "specification/aie2ps/isa.h"

namespace aiebu {

class aie2ps_encoder : public encoder
{
  std::shared_ptr<std::map<std::string, std::shared_ptr<isa_op>>> m_isa;
  std::vector<std::shared_ptr<writer>> twriter;
public:
  aie2ps_encoder() {     
    isa i;
    m_isa = i.get_isamap();
  }

  virtual std::vector<std::shared_ptr<writer>>
  process(std::shared_ptr<preprocessed_output> input) override;
  virtual std::string get_TextSectionName(uint32_t colnum, pageid_type pagenum) {return ".ctrltext." + std::to_string(colnum) + "." + std::to_string(pagenum); }
  virtual std::string get_DataSectionName(uint32_t colnum, pageid_type pagenum) {return ".ctrldata." + std::to_string(colnum) + "." + std::to_string(pagenum); }
  virtual std::string get_PadSectionName(uint32_t colnum) {return ".pad." + std::to_string(colnum); }
  std::string page_writer(page& lpage, std::map<std::string, std::shared_ptr<scratchpad_info>>& scratchpad, std::map<std::string, uint32_t>& labelpageindex, uint32_t control_packet_index);
  virtual void patch57(const std::shared_ptr<section_writer> textwriter, std::shared_ptr<section_writer> datawriter, offset_type offset, uint64_t patch);
  void fill_scratchpad(std::shared_ptr<section_writer> padwriter,const std::map<std::string, std::shared_ptr<scratchpad_info>>& scratchpads);
  void fill_control_packet_symbols(std::shared_ptr<section_writer> padwriter,const uint32_t col, const std::string& controlpacket_padname, std::vector<symbol>& syms, const std::map<std::string, std::shared_ptr<scratchpad_info>>& scratchpads);
  std::string findKey(const std::map<std::string, std::vector<std::string>>& myMap, const std::string& value);

  virtual std::shared_ptr<assembler_state>
  create_assembler_state(std::shared_ptr<std::map<std::string, std::shared_ptr<isa_op>>> isa,
                         std::vector<std::shared_ptr<asm_data>>& data,
                         std::map<std::string, std::shared_ptr<scratchpad_info>>& scratchpad,
                         std::map<std::string, uint32_t>& labelpageindex,
                         uint32_t control_packet_index, bool makeunique)
  {
    return std::make_shared<assembler_state_aie2ps>(isa, data, scratchpad, labelpageindex, control_packet_index, makeunique);
  }

  std::vector<std::shared_ptr<writer>> get_writers() { return twriter; }
};

class aie2ps_config_encoder : public aie2ps_encoder
{

public:
  std::string get_TextSectionName(uint32_t colnum, pageid_type pagenum/*, uint32_t group*/) override {return ".ctrltext." + std::to_string(colnum) + "." + std::to_string(pagenum);/* + "." + std::to_string(group);*/ }
  std::string get_DataSectionName(uint32_t colnum, pageid_type pagenum/*, uint32_t group*/) override {return ".ctrldata." + std::to_string(colnum) + "." + std::to_string(pagenum);/* + "." + std::to_string(group);*/ }
  std::string get_PadSectionName(uint32_t colnum/*, uint32_t group*/) override {return ".pad." + std::to_string(colnum);/* + "." + std::to_string(group);*/ }
};

//asm_config_preprocessor<aie2ps_config_encoder, aie2ps_preprocessed_output>
template <typename T, typename input_tamplete>
class asm_config_encoder : public encoder
{
  std::vector<std::shared_ptr<writer>> twriter;
public:
  //std::shared_ptr<asm_config_preprocessed_output<input_tamplete>>
  virtual std::vector<std::shared_ptr<writer>>
  process(std::shared_ptr<preprocessed_output> input) override
  {
    auto output_writer = std::make_shared<config_writer>();
    twriter.push_back(output_writer);
    auto tinput = std::static_pointer_cast<asm_config_preprocessed_output<input_tamplete>>(input);

    for (auto& [kernel, instances] : tinput->m_output) {
      for(auto& [iname, instance] : instances)
      {
        T encoder_object;
        output_writer->m_output[kernel][iname] = std::move(encoder_object.process(instance));
      }
    }
    return twriter;
    //return encoder_object.get_writers();
  }
};
}
#endif //_AIEBU_ENCODER_AIE2PS_ENCODER_H_
