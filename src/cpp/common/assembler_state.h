// SPDX-License-Identifier: MIT
// Copyright (C) 2024-2025, Advanced Micro Devices, Inc. All rights reserved.

#ifndef _AIEBU_COMMON_ASSEMBLER_STATE_H_
#define _AIEBU_COMMON_ASSEMBLER_STATE_H_

#include "asm/asm_parser.h"
#include "oparg.h"
#include "utils.h"
#include "symbol.h"
#include "ops.h"
#include <iostream>
#include <memory>
#include <map>

namespace aiebu {

class isa_op;

class symbol_exception : public std::exception
{

};

class job
{
  jobid_type m_jobid;
  offset_type m_start;
  offset_type m_end;
  uint32_t m_start_index;
  uint32_t m_end_index;
  pageid_type m_pagenum;
  bool m_isdeferred;
  uint32_t m_eopnum;

public:
  std::vector<barrierid_type> m_barrierids;
  std::vector<jobid_type> m_dependentjobs;

  job(jobid_type jobid, offset_type pos, uint32_t index, uint32_t eopnum, bool isdeferred)
  :m_jobid(jobid), m_start(pos), m_end(pos), m_start_index(index), m_end_index(index),
   m_pagenum(NO_PAGE), m_isdeferred(isdeferred), m_eopnum(eopnum) {}

  job(job* j)
  {
    j->m_jobid = m_jobid;
    j->m_start = m_start;
    j->m_end = m_end;
    j->m_start_index = m_start_index;
    j->m_end_index = m_end_index;
    j->m_pagenum = m_pagenum;
    j->m_isdeferred = m_isdeferred;
    j->m_eopnum = m_eopnum;
  }

  HEADER_ACCESS_GET_SET(jobid_type, jobid);
  HEADER_ACCESS_GET_SET(pageid_type, pagenum);
  HEADER_ACCESS_GET_SET(uint32_t, eopnum);
  HEADER_ACCESS_GET_SET(offset_type, start);
  HEADER_ACCESS_GET_SET(offset_type, end);
  HEADER_ACCESS_GET_SET(uint32_t, start_index);
  HEADER_ACCESS_GET_SET(uint32_t, end_index);
  offset_type get_size()
  {
    return m_end - m_start;
  }
};

class label
{
  std::string m_name;
  offset_type m_pos;
  uint32_t m_index;
  int32_t m_count = -1;
  uint64_t m_size = 0;
  pageid_type m_pagenum;

public:

  label(std::string& name, offset_type pos, uint32_t index):m_name(name), m_pos(pos), m_index(index) {}
  label(label *t)
  {
    t->m_name = m_name;
    t->m_pos = m_pos;
    t->m_index = m_index;
    t->m_pagenum = m_pagenum;
  }

  HEADER_ACCESS_GET_SET(pageid_type, pagenum);
  HEADER_ACCESS_GET_SET(uint64_t, size);
  HEADER_ACCESS_GET_SET(int32_t, count);
  HEADER_ACCESS_GET_SET(uint32_t, index);
  HEADER_ACCESS_GET_SET(offset_type, pos);
  const std::string& get_name() { return m_name; }
  void increment_size(uint64_t size)
  {
    m_size += size;
  }

  void increment_count(int32_t count)
  {
    m_count += count;
  }
};

class assembler_state : public std::enable_shared_from_this<assembler_state>
{
protected:
  offset_type m_pos = 0;

  inline std::string gen_label_name(bool makeunique, const std::shared_ptr<asm_data> data)
  {
    return makeunique ? data->get_file() + ":" + data->get_operation()->get_name() : data->get_operation()->get_name();
  }

  inline std::string gen_job_name(bool makeunique, const std::shared_ptr<asm_data> data)
  {
    return makeunique ? data->get_file() + ":" + data->get_operation()->get_args()[0] : data->get_operation()->get_args()[0];
  }

  inline std::string gen_eop_name(uint32_t eopnum)
  {
    return EOP_ID + std::to_string(eopnum);
  }

  struct ActionId {
    uint32_t actor_start; //offset in string
    uint32_t base_actor_offset; //base channel
  };

  //std::unordered_map<std::string, ActionId> actor_id;

  virtual std::unordered_map<std::string, ActionId>& get_actor_id_map() const = 0;
  uint32_t get_actor(const std::string& prefix, const std::string& s) const
  {
    std::unordered_map<std::string, ActionId>& actor_id = get_actor_id_map();
    uint32_t actor = std::stoi(s.substr(actor_id.at(prefix).actor_start));
    return actor_id.at(prefix).base_actor_offset + actor;
  }

  assembler_state(std::shared_ptr<std::map<std::string, std::shared_ptr<isa_op>>> isa,
                  std::vector<std::shared_ptr<asm_data>>& data,
                  std::map<std::string, std::shared_ptr<scratchpad_info>>& scratchpad,
                  std::map<std::string, uint32_t>& labelpageindex, uint32_t control_packet_index, bool makeunique);
  assembler_state(const assembler_state& rhs) = default;
  assembler_state& operator=(const assembler_state& rhs) = default;
  assembler_state(assembler_state &&s) = default;
public:
  std::shared_ptr<std::map<std::string, std::shared_ptr<isa_op>>> m_isa;
  std::vector<std::shared_ptr<asm_data>>& m_data;
  std::vector<jobid_type> m_jobids;
  std::map<jobid_type, std::shared_ptr<job>> m_jobmap;
  std::map<std::string, std::shared_ptr<label>> m_labelmap;
  std::map<barrierid_type, std::vector<jobid_type>> m_localbarriermap;
  std::map<jobid_type, std::vector<jobid_type>> m_joblaunchmap;
  std::map<std::string, std::shared_ptr<scratchpad_info>>& m_scratchpad;
  std::map<std::string, std::vector<std::string>> m_patch;
  std::map<std::string, uint32_t>& m_labelpageindex;
  uint32_t m_control_packet_index;
  std::string m_controlpacket_padname;

  HEADER_ACCESS_GET_SET(offset_type, pos);

  void printstate() const;

  bool is_number(const std::string& s) const {
    return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
  }

  bool containscratchpads(const std::string& label) const
  {
    return (m_scratchpad.find(label) != m_scratchpad.end());
  }

  uint32_t parse_num_arg(const std::string& str);

  void process(bool makeunique);

  const std::vector<jobid_type> get_job_list() const
  {
    // get list of jobs
    std::vector<jobid_type> keys;
    std::transform(
      m_jobmap.begin(),
      m_jobmap.end(),
      std::back_inserter(keys),
      [](const std::map<jobid_type, std::shared_ptr<job>>::value_type &pair){return pair.first;});
    return keys;
  }
  const std::vector<std::string> get_label_list() const
  {
    // get list of jobs
    std::vector<std::string> keys;
    std::transform(
      m_labelmap.begin(),
      m_labelmap.end(),
      std::back_inserter(keys),
      [](const std::map<std::string, std::shared_ptr<label>>::value_type &pair){return pair.first;});
    return keys;
  }

  virtual symbol::patch_schema get_shim_dma_patching() const = 0;
  virtual symbol::patch_schema get_control_packet_patching() const = 0;
  virtual ~assembler_state() = default;
};


class assembler_state_aie2ps : public assembler_state
{
  std::unordered_map<std::string, ActionId>&
  get_actor_id_map() const override
  {
    static std::unordered_map<std::string, ActionId> actor_id = {
      {"mm2s", {5, 6}},          //NOLINT
      {"s2mm", {5, 0}},          //NOLINT
      {"tile_mm2s", {10, 6}},    //NOLINT
      {"tile_s2mm", {10, 0}},    //NOLINT
      {"shim_mm2s", {10, 6}},    //NOLINT
      {"shim_s2mm", {10, 0}},    //NOLINT
      {"mem_mm2s", {9, 6}},      //NOLINT
      {"mem_s2mm", {9, 0}}       //NOLINT
    };
    return actor_id;
  }
  public:
  assembler_state_aie2ps(std::shared_ptr<std::map<std::string, std::shared_ptr<isa_op>>> isa,
                         std::vector<std::shared_ptr<asm_data>>& data,
                         std::map<std::string, std::shared_ptr<scratchpad_info>>& scratchpad,
                         std::map<std::string, uint32_t>& labelpageindex, uint32_t control_packet_index, bool makeunique)
                  : assembler_state(isa, data, scratchpad, labelpageindex, control_packet_index, makeunique)
  {
    //shim_dma_patching = symbol::patch_schema::shim_dma_57;
    //control_packet_patching = symbol::patch_schema::control_packet_57;
  }

  symbol::patch_schema get_shim_dma_patching() const override { return symbol::patch_schema::shim_dma_57; }
  symbol::patch_schema get_control_packet_patching() const override { return symbol::patch_schema::control_packet_57; }
};

class assembler_state_aie4 : public assembler_state
{
  std::unordered_map<std::string, ActionId>&
  get_actor_id_map() const override
  {
    static std::unordered_map<std::string, ActionId> actor_id = {
      {"mm2s", {5, 6}},               //NOLINT
      {"s2mm", {5, 0}},               //NOLINT
      {"tile_mm2s", {10, 6}},         //NOLINT
      {"tile_s2mm", {10, 0}},         //NOLINT
      {"shim_mm2s", {10, 6}},         //NOLINT
      {"shim_s2mm", {10, 0}},         //NOLINT
      {"mem_mm2s", {9, 16}},          //NOLINT
      {"mem_s2mm", {9, 0}},           //NOLINT
      {"shim_ctrl_mm2s", {15, 16}}    //NOLINT
    };
    return actor_id;
  }

  public:
  assembler_state_aie4(std::shared_ptr<std::map<std::string, std::shared_ptr<isa_op>>> isa,
                       std::vector<std::shared_ptr<asm_data>>& data,
                       std::map<std::string, std::shared_ptr<scratchpad_info>>& scratchpad,
                       std::map<std::string, uint32_t>& labelpageindex, uint32_t control_packet_index, bool makeunique)
                  : assembler_state(isa, data, scratchpad, labelpageindex, control_packet_index, makeunique)
  {
    //shim_dma_patching = symbol::patch_schema::shim_dma_57_aie4;
    //control_packet_patching = symbol::patch_schema::control_packet_57_aie4;
  }

  symbol::patch_schema get_shim_dma_patching() const override { return symbol::patch_schema::shim_dma_57_aie4; }
  symbol::patch_schema get_control_packet_patching() const override { return symbol::patch_schema::control_packet_57_aie4; }
};
}
#endif //_AIEBU_COMMON_ASSEMBLER_STATE_H_
