// SPDX-License-Identifier: MIT
// Copyright (C) 2025, Advanced Micro Devices, Inc. All rights reserved.

#ifndef AIEBU_ENCODER_AIE2PS_REPORT_H_
#define AIEBU_ENCODER_AIE2PS_REPORT_H_
#include <map>
#include <memory>
#include <vector>
#include "utils.h"
#include "assembler_state.h"
#include "asm/page.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace aiebu {

class Line {
public:
    Line(uint32_t linenumber, offset_type high_pc, offset_type low_pc, const std::string& opcode)
        : m_linenumber(linenumber), m_highpc(high_pc), m_lowpc(low_pc), m_opcode(opcode) {}

    uint32_t get_linenumber() const { return m_linenumber; }
    offset_type get_highpc() const { return m_highpc; }
    offset_type get_lowpc() const { return m_lowpc; }
    std::string get_opcode() const { return m_opcode; }

    friend std::ostream& operator<<(std::ostream& os, const Line& line) {
        os << "linenumber:" << line.m_linenumber << " high_pc:" << line.m_highpc
           << " low_pc:" << line.m_lowpc << " opcode:" << line.m_opcode;
        return os;
    }

    boost::property_tree::ptree to_ptree(int sno, uint32_t column, pageid_type page_num, const std::string& filename) const {
        boost::property_tree::ptree pt;
        pt.put("sno", sno);
        pt.put("operation", m_opcode);
        pt.put("opcode_size", m_highpc-m_lowpc+1);
        pt.put("column", column);
        pt.put("page_index", page_num);
        pt.put("page_offset", m_lowpc);
        pt.put("line", m_linenumber);
        pt.put("file", filename);
        return pt;
    }
private:
    uint32_t m_linenumber;
    offset_type m_highpc;
    offset_type m_lowpc;
    std::string m_opcode;
};

class Function {
private:
    std::string m_filename;
    std::string m_name;
    uint32_t m_colnum;
    pageid_type m_pagenum;
    offset_type m_highpc;
    offset_type m_lowpc;
    std::vector<std::shared_ptr<Line>> m_textlines;
    std::vector<std::shared_ptr<Line>> m_datalines;
public:
    Function(const std::string& filename, const std::string& name, offset_type high_pc, offset_type low_pc, uint32_t col, pageid_type pagenum)
        : m_filename(filename), m_name(name), m_colnum(col), m_pagenum(pagenum), m_highpc(high_pc), m_lowpc(low_pc) {}

    void add_textline(std::shared_ptr<Line> line) { m_textlines.emplace_back(line); }
    void add_dataline(std::shared_ptr<Line> line) { m_datalines.emplace_back(line); }

    const std::vector<std::shared_ptr<Line>>& get_textlines() const { return m_textlines; }
    const std::vector<std::shared_ptr<Line>>& get_datalines() const { return m_datalines; }

    std::string get_filename() const { return m_filename; }
    std::string get_name() const { return m_name; }
    uint32_t get_column() const { return m_colnum; }
    uint32_t get_pagenum() const { return m_pagenum; }
    pageid_type get_highPc() const { return m_highpc; }
    pageid_type get_lowPc() const { return m_lowpc; }

    friend std::ostream& operator<<(std::ostream& os, const Function& func) {
        os << "\tfilename:" << func.m_filename << " name:" << func.m_name << " col_num:" << func.m_colnum
           << " page:" << func.m_pagenum << " high_pc:" << func.m_highpc << " low_pc:" << func.m_lowpc << " line:\n";
        for (const auto& line : func.m_textlines) {
            os << "\t\t" << *line << "\n";
        }
        for (const auto& line : func.m_datalines) {
            os << "\t\t" << *line << "\n";
        }
        return os;
    }

};

class Debug {
public:
    std::string add_function(const std::string& filename, const std::string& name, offset_type high_pc, offset_type low_pc, uint32_t col, pageid_type pagenum) {
        std::string key = filename + std::to_string(col) + name;
        functions[key] = std::make_shared<Function>(filename, name, high_pc, low_pc, col, pagenum);
        insertion_order.push_back(key);
        return key;
    }

    void add_textline(const std::string& func, uint32_t linenumber, offset_type high_pc, offset_type low_pc, const std::string& token) {
        functions.at(func)->add_textline(std::make_shared<Line>(linenumber, high_pc, low_pc, token));
    }

    void add_dataline(const std::string& func, uint32_t linenumber, offset_type high_pc, offset_type low_pc, const std::string& token) {
        functions.at(func)->add_dataline(std::make_shared<Line>(linenumber, high_pc, low_pc, token));
    }

    friend std::ostream& operator<<(std::ostream& os, const Debug& debug) {
        for (const auto& key : debug.insertion_order) {
            os << *debug.functions.at(key) << " ";
        }
        return os;
    }

    boost::property_tree::ptree to_ptree() const {
        boost::property_tree::ptree pt;
        boost::property_tree::ptree lines;
        int sno = 1; // Example serial number
        for (const auto& key : insertion_order) {
            const auto& func = functions.at(key);
            for (const auto& line : func->get_textlines()) {
                lines.push_back(std::make_pair("", line->to_ptree(sno++, func->get_column(), func->get_pagenum(), func->get_filename())));
            }
            for (const auto& line : func->get_datalines()) {
                lines.push_back(std::make_pair("", line->to_ptree(sno++, func->get_column(), func->get_pagenum(), func->get_filename())));
            }
        }
        pt.push_back(std::make_pair("debug", lines));
        return pt;
    }

private:
    std::map<std::string,std::shared_ptr<Function>> functions;
    std::vector<std::string> insertion_order;
};

class aie2ps_report
{
  class report_page
  {
    public:
    uint32_t m_colnum;
    pageid_type m_pagenum;
    offset_type m_textsize;
    offset_type m_datasize;
    std::vector<jobid_type> m_jobids;
    std::map<barrierid_type, std::vector<jobid_type>> m_localbarriermap;
    std::map<jobid_type, std::vector<jobid_type>> m_joblaunchmap;


    report_page(uint32_t colnum, pageid_type pagenum, offset_type textsize, offset_type datasize, std::vector<jobid_type> jobids, std::map<barrierid_type, std::vector<jobid_type>> localbarriermap, std::map<jobid_type, std::vector<jobid_type>> joblaunchmap):
      m_colnum(colnum),
      m_pagenum(pagenum),
      m_textsize(textsize),
      m_datasize(datasize),
      m_jobids(jobids),
      m_localbarriermap(localbarriermap),
      m_joblaunchmap(joblaunchmap) { }


    report_page(const report_page& rhs) = default;
    report_page& operator=(const report_page& rhs) = default;
    report_page(report_page &&s) = default;
  };

  std::string m_build_id = LATEST_COMMIT_HASH;
  std::map<uint32_t, std::vector<report_page>> m_colpages;

  public:

  void addpage(page& lpage, assembler_state &page_state, offset_type textsize, offset_type datasize);
  std::vector<char> generate();
};

}
#endif //AIEBU_ENCODER_AIE2PS_REPORT_H_
