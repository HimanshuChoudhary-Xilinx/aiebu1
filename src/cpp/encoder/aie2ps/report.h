// SPDX-License-Identifier: MIT
// Copyright (C) 2025, Advanced Micro Devices, Inc. All rights reserved.

#ifndef AIEBU_ENCODER_AIE2PS_REPORT_H_
#define AIEBU_ENCODER_AIE2PS_REPORT_H_


#include <string>
#include <vector>
#include <memory>
#include <map>
#include <iostream>
#include "json/nlohmann/json.hpp"
#include "utils.h"
#include "assembler_state.h"
#include "asm/page.h"
#include "version.h"

namespace aiebu {

using json = nlohmann::json;

// Class hold info for each line in jod/function
class Line {
public:
  Line(uint32_t linenumber, offset_type high_pc, offset_type low_pc, const std::string& opcode, int annotation_index)
      : m_linenumber(linenumber), m_highpc(high_pc), m_lowpc(low_pc), m_opcode(opcode), m_annotation_index(annotation_index) {}

  json to_json(int sno, uint32_t column, pageid_type page_num, const std::string& filename, const std::vector<annotation_type>& annotations) const {
    json j = {
             {"sno", sno},
             {"operation", m_opcode},
             {"opcode_size", m_highpc - m_lowpc + 1},
             {"column", column},
             {"page_index", page_num},
             {"page_offset", m_lowpc},
             {"line", m_linenumber},
             {"file", filename}
        };

    if (m_annotation_index != -1 && m_annotation_index < static_cast<int>(annotations.size())) {
      j["annotation"] = {
                {"id", annotations[m_annotation_index].get_id()},
                {"name", annotations[m_annotation_index].get_name()},
                {"description", annotations[m_annotation_index].get_description()}
        };
    }
    return j;
  }

private:
  uint32_t m_linenumber;
  offset_type m_highpc;
  offset_type m_lowpc;
  std::string m_opcode;
  int m_annotation_index;
};

// Class hold info for each job/function
class Function {
public:

  Function(const std::string& filename, const std::string& name, offset_type high_pc, offset_type low_pc, uint32_t col, pageid_type pagenum)
        : m_filename(filename), m_name(name), m_colnum(col), m_pagenum(pagenum), m_highpc(high_pc), m_lowpc(low_pc) {}

  void add_textline(std::shared_ptr<Line> line) { m_textlines.push_back(std::move(line)); }
  void add_dataline(std::shared_ptr<Line> line) { m_datalines.push_back(std::move(line)); }

  const std::vector<std::shared_ptr<Line>>& get_textlines() const { return m_textlines; }
  const std::vector<std::shared_ptr<Line>>& get_datalines() const { return m_datalines; }

  const std::string& get_filename() const { return m_filename; }
  const std::string& get_name() const { return m_name; }
  uint32_t get_column() const { return m_colnum; }
  pageid_type get_pagenum() const { return m_pagenum; }
  offset_type get_highPc() const { return m_highpc; }
  offset_type get_lowPc() const { return m_lowpc; }

private:
  std::string m_filename, m_name;
  uint32_t m_colnum;
  pageid_type m_pagenum;
  offset_type m_highpc, m_lowpc;
  std::vector<std::shared_ptr<Line>> m_textlines, m_datalines;
};

// Class to generate debug section for tracing
class Debug {
public:
  void set_annotations(std::vector<annotation_type> annotations) {
    m_annotation_list = std::move(annotations);
  }

  std::string add_function(const std::string& filename, const std::string& name, offset_type high_pc, offset_type low_pc, uint32_t col, pageid_type pagenum) {
    std::string key = filename + std::to_string(col) + name;
    functions[key] = std::make_shared<Function>(filename, name, high_pc, low_pc, col, pagenum);
    insertion_order.push_back(key);
    return key;
  }

  void add_textline(const std::string& func, uint32_t line, offset_type hi, offset_type lo, const std::string& opcode, int ann) {
    functions.at(func)->add_textline(std::make_shared<Line>(line, hi, lo, opcode, ann));
  }

  void add_dataline(const std::string& func, uint32_t line, offset_type hi, offset_type lo, const std::string& opcode, int ann) {
    functions.at(func)->add_dataline(std::make_shared<Line>(line, hi, lo, opcode, ann));
  }

  json to_json() const {
    json lines_json = json::array();
    int sno = 1;
    for (const auto& key : insertion_order) {
      const auto& func = functions.at(key);
      for (const auto& line : func->get_textlines()) {
        lines_json.push_back(line->to_json(sno++, func->get_column(), func->get_pagenum(), func->get_filename(), m_annotation_list));
      }
      for (const auto& line : func->get_datalines()) {
        lines_json.push_back(line->to_json(sno++, func->get_column(), func->get_pagenum(), func->get_filename(), m_annotation_list));
      }
    }
    return {{"debug", lines_json}};
  }

private:
  std::map<std::string, std::shared_ptr<Function>> functions;
  std::vector<std::string> insertion_order;
  std::vector<annotation_type> m_annotation_list;
};


// Class to generate asm report summary
class asm_report {
  class report_page {
  public:
    uint32_t m_colnum;
    pageid_type m_pagenum;
    offset_type m_textsize;
    offset_type m_datasize;
    std::vector<jobid_type> m_jobids;
    std::map<barrierid_type, std::vector<jobid_type>> m_localbarriermap;
    std::map<jobid_type, std::vector<jobid_type>> m_joblaunchmap;

    report_page(uint32_t colnum, pageid_type pagenum, offset_type textsize, offset_type datasize,
                const std::vector<jobid_type>& jobids,
                const std::map<barrierid_type, std::vector<jobid_type>>& barriermap,
                const std::map<jobid_type, std::vector<jobid_type>>& launchmap)
        : m_colnum(colnum), m_pagenum(pagenum), m_textsize(textsize), m_datasize(datasize),
          m_jobids(jobids), m_localbarriermap(barriermap), m_joblaunchmap(launchmap) {}

    json to_json() const {
      json j;
      j["column"] = m_colnum;
      j["page_index"] = m_pagenum;
      j["text_size"] = m_textsize;
      j["data_size"] = m_datasize;
      j["job_ids"] = m_jobids;

      for (const auto& [bid, jobs] : m_localbarriermap) {
        j["local_barriers"][std::to_string(bid)] = jobs;
      }
      for (const auto& [jid, deps] : m_joblaunchmap) {
        j["launch_dependencies"][jid] = deps;
      }
      return j;
    }
  };

  std::string m_build_id = aiebu_build_version_hash;
  std::map<uint32_t, std::vector<report_page>> m_colpages;
public:
  void addpage(page& lpage, std::shared_ptr<assembler_state> page_state, offset_type textsize, offset_type datasize); // Implementation assumed

  void summary(std::ostream& output);

  json to_json() const {
    json j;
    j["build_id"] = m_build_id;
    json colpages_json;

    for (const auto& [col, pages] : m_colpages) {
      json pages_array = json::array();
      for (const auto& pg : pages) {
        pages_array.push_back(pg.to_json());
      }
      colpages_json[std::to_string(col)] = pages_array;
    }

    j["columns"] = colpages_json;
    return j;
  }
};

}
#endif //AIEBU_ENCODER_AIE2PS_REPORT_H_
