// SPDX-License-Identifier: MIT
// Copyright (C) 2024-2025, Advanced Micro Devices, Inc. All rights reserved.
#ifndef AIEBU_PREPROCESSOR_ASM_ASM_PARSER_H_
#define AIEBU_PREPROCESSOR_ASM_ASM_PARSER_H_

#include "code_section.h"
#include "utils.h"
#include "file_utils.h"
#include "logger.h"
#include "common/regex_wrapper.h"
#include "logger.h"

#include <map>
#include <memory>
#include <set>
#include <stack>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace aiebu {

inline std::string trim(const std::string& line)
{
    // trim the last
    std::string WhiteSpace = " \t\f\v\n\r";
    std::size_t start = line.find_first_not_of(WhiteSpace);
    std::size_t end = line.find_last_not_of(WhiteSpace);
    return (start == end && start == std::string::npos) ? std::string() : line.substr(start, end - start + 1);
}

inline void verify_match(const smatch& sm, error::error_code ecode, const char* msg)
{
  if (sm.size() < 3 || !sm[2].matched || sm[2].length() == 0)
    throw error(ecode, msg);
}

enum class operation_type: uint8_t
{
  label = 1,
  op = 2,
  annotation = 3,
};

// Start defining regexs required for parsing aie2asm

const std::string HEX_RE("[[:space:]]*(0[xX][[:xdigit:]]+)[[:space:]]*");
const std::string L_BRACK_RE("[[:space:]]*\\([[:space:]]*");
const std::string R_BRACK_RE("[[:space:]]*\\)[[:space:]]*");

class operation
{
  std::string m_name;
  std::vector<std::string> m_args;
public:

  operation(const std::string& name, std::string sargs): m_name(name)
  {
    std::transform(m_name.begin(), m_name.end(), m_name.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    std::transform(sargs.begin(), sargs.end(), sargs.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

    std::string s;
    std::stringstream ss(sargs);
    while (std::getline(ss, s, ' ')) {
      if (s.size() == 0)
        continue;
      m_args.emplace_back(s.substr(0, s.find_last_not_of(",")+1));
    }
  }

  const std::string& get_name() const { return m_name; }
  const std::vector<std::string>& get_args() const { return m_args; }
};

class asm_parser;
class directive
{
public:
  std::shared_ptr<asm_parser> m_parserptr;
public:
  directive() {}
  virtual void operate(std::shared_ptr<asm_parser> parserptr, const smatch& sm) = 0;
  virtual ~directive() = default;
};

class attach_to_group_directive: public directive
{
public:
  attach_to_group_directive() = default;
  void operate(std::shared_ptr<asm_parser> parserptr, const smatch& sm) override;
  ~attach_to_group_directive() override = default;
};

class include_directive: public directive
{
  bool read_include_file(std::string filename);
public:
  include_directive() = default;
  void operate(std::shared_ptr<asm_parser> parserptr, const smatch& sm) override;
  ~include_directive() override = default;
};

class end_of_label_directive: public directive
{
public:
  end_of_label_directive() = default;
  void operate(std::shared_ptr<asm_parser> parserptr, const smatch& sm) override;
  ~end_of_label_directive() override = default;
};

class pad_directive: public directive
{
  bool read_pad_file(std::string& name, std::string& filename);
public:
  offset_type convert2int(std::string& str)
  {
    offset_type size = 0;
    try {
      size_t pos;
      size = std::stoul(str, &pos, 0);
      if (pos != str.length()) {
        throw error(error::error_code::internal_error,"Invalid characters found in the scratchpad size");
      }
    } catch (const std::invalid_argument&) {
      throw error(error::error_code::internal_error, "Invalid argument in scratchpad size");
    } catch (const std::out_of_range&) {
      throw error(error::error_code::internal_error, "Out of range scratchpad size");
    }
    return size;
  }
  void add_scratchpad(std::string& name, std::string& str);
  pad_directive() = default;
  void operate(std::shared_ptr<asm_parser> parserptr, const smatch& sm) override;
  ~pad_directive() override = default;
};

class section_directive: public directive
{
  bool is_test_section(const std::string& str) {return !str.substr(0,9).compare(".ctrltext"); }
  bool is_data_section(const std::string& str) {return !str.substr(0,9).compare(".ctrldata"); }
  bool is_annotation_section(const std::string& str) {return !str.substr(0,10).compare("annotation"); }
public:
  section_directive() = default;
  void operate(std::shared_ptr<asm_parser> parserptr, const smatch& sm) override;
  ~section_directive() override = default;
};

class partition_directive: public directive
{
public:
  partition_directive() = default;
  void operate(std::shared_ptr<asm_parser> parserptr, const smatch& sm) override;
  ~partition_directive() override = default;
  partition_directive(const partition_directive&) = default;
  partition_directive& operator=(const partition_directive&) = default;
  partition_directive(partition_directive&&) = default;
  partition_directive& operator=(partition_directive&&) = default;
};

class target_directive: public directive
{
public:
  target_directive() = default;
  void operate(std::shared_ptr<asm_parser> parserptr, const smatch& sm) override;
  ~target_directive() override = default;
  target_directive(const target_directive&) = default;
  target_directive& operator=(const target_directive&) = default;
  target_directive(target_directive&&) = default;
  target_directive& operator=(target_directive&&) = default;
};

class aie_row_topology_directive: public directive
{
public:
  aie_row_topology_directive() = default;
  void operate(std::shared_ptr<asm_parser> parserptr, const smatch& sm) override;
  ~aie_row_topology_directive() override = default;
  aie_row_topology_directive(const aie_row_topology_directive&) = default;
  aie_row_topology_directive& operator=(const aie_row_topology_directive&) = default;
  aie_row_topology_directive(aie_row_topology_directive&&) = default;
  aie_row_topology_directive& operator=(aie_row_topology_directive&&) = default;
};

class asm_data
{
  std::shared_ptr<operation> m_op;
  operation_type m_optype;
  code_section m_section;
  offset_type m_size;
  pageid_type m_pagenum;
  uint32_t m_linenumber;
  std::string m_line;
  std::string m_file;
  int m_annotation_index = -1;

public:
  asm_data() = default;
  asm_data(std::shared_ptr<operation> op, operation_type optype,
           code_section sec, offset_type size, uint32_t pgnum,
           uint32_t ln, std::string line, std::string file)
           :m_op(op), m_optype(optype), m_section(sec), m_size(size),
            m_pagenum(pgnum), m_linenumber(ln), m_line(line), m_file(file) {}

  asm_data( asm_data* a)
  {
    a->m_op = m_op;
    a->m_optype = m_optype;
    a->m_section = m_section;
    a->m_size = m_size;
    a->m_pagenum = m_pagenum;
    a->m_linenumber = m_linenumber;
    a->m_line = m_line;
    a->m_file = m_file;
  }

  HEADER_ACCESS_GET_SET(code_section, section);
  HEADER_ACCESS_GET_SET(offset_type, size);
  HEADER_ACCESS_GET_SET(pageid_type, pagenum);
  HEADER_ACCESS_GET_SET(uint32_t, linenumber);
  HEADER_ACCESS_GET_SET(std::string, file);
  HEADER_ACCESS_GET_SET(std::string, line);
  bool isLabel() { return m_optype == operation_type::label; }
  bool isOpcode() { return m_optype == operation_type::op; }
  bool isAnnotation() { return m_optype == operation_type::annotation; }
  std::shared_ptr<operation> get_operation() { return m_op; }
  void set_operation(std::shared_ptr<operation> op) { m_op = op; }
  int get_annotation_index() { return m_annotation_index; }
  void set_annotation_index(int annotation_index) { m_annotation_index = annotation_index; }
};

class section_asmdata
{
public:
  std::vector<std::shared_ptr<asm_data>> text;
  std::vector<std::shared_ptr<asm_data>> data;
};

class scratchpad_info
{
  std::string m_name;
  offset_type m_size;
  offset_type m_offset;
  offset_type m_base;
  std::vector<char> m_content;
public:
  scratchpad_info(std::string name, offset_type size, offset_type offset, offset_type base, std::vector<char>& content):
    m_name(name), m_size(size), m_offset(offset), m_base(base), m_content(std::move(content)) {}

  scratchpad_info(const scratchpad_info& other) : m_name(other.m_name),
    m_size(other.m_size), m_offset(other.m_offset), m_base(other.m_base),
    m_content(other.m_content) { }

  HEADER_ACCESS_GET_SET(std::string, name);
  HEADER_ACCESS_GET_SET(offset_type, size);
  HEADER_ACCESS_GET_SET(offset_type, offset);
  HEADER_ACCESS_GET_SET(offset_type, base);

  const std::vector<char>& get_content() const { return m_content; }
};

class annotation_type
{
  std::string m_id;
  std::string m_name;
  std::string m_description;

  public:
  annotation_type(std::string id, std::string name, std::string description)
    : m_id(std::move(id)), m_name(std::move(name)), m_description(std::move(description)) { /*std::cout << m_id << " " << m_name << " " << m_description << "|\n";*/ }

  annotation_type(const annotation_type& rhs) = default;
  annotation_type& operator=(const annotation_type& rhs) = default;
  annotation_type(annotation_type &&s) = default;

  std::string get_id() const { return m_id; }
  std::string get_name() const { return m_name; }
  std::string get_description() const { return m_description; }
};

class col_data
{
  std::map<std::string, section_asmdata> m_label_data;
  std::map<std::string, uint32_t> m_labelpageindex;
  std::map<std::string, std::shared_ptr<scratchpad_info>> m_scratchpads;
  // Counter for PREEMPT opcodes in this column's control code.
  // Used to verify all columns have the same number of preempt points.
  uint32_t m_preempt_count = 0;
public:

  std::vector<std::shared_ptr<asm_data>> get_label_asmdata(const std::string& label)
  {
    std::vector<std::shared_ptr<asm_data>> result(m_label_data[label].text);
    result.insert(result.end(), m_label_data[label].data.begin(), m_label_data[label].data.end());
    return result;
  }

  std::map<std::string, section_asmdata>& get_label_data() { return m_label_data; }

  std::map<std::string, uint32_t>& get_labelpageindex() { return m_labelpageindex; }

  void set_labelpageindex(std::string& label, uint32_t val) { m_labelpageindex[label] = val; }

  std::map<std::string, std::shared_ptr<scratchpad_info>>& get_scratchpads() { return m_scratchpads; }
  void set_scratchpad(std::string& name, offset_type size, std::vector<char>& content)
  {
    m_scratchpads[name] = std::make_shared<scratchpad_info>(name, size, 0 , 0, content);
  }

  // Get the number of PREEMPT opcodes encountered in this column's control code
  uint32_t get_preempt_count() const { return m_preempt_count; }

  // Increment the PREEMPT opcode counter (called when parsing encounters a PREEMPT opcode)
  void increment_preempt_count() { m_preempt_count++; }
};

class attach_to_group_directive;
class include_directive;
class end_of_label_directive;
class pad_directive;
class section_directive;
class partition_directive;
class target_directive;
class aie_row_topology_directive;

class asm_parser: public std::enable_shared_from_this<asm_parser>
{
  std::unordered_map<uint32_t, col_data> m_col;
  const std::vector<char> &m_data;
  std::map<std::string, std::shared_ptr<directive>> directive_list;
  std::stack<bool> isdatastack;
  std::string m_current_label = "default";
  int m_current_col = -1;
  const std::vector<std::string>& m_include_list;
  bool annotation_state = false;
  std::vector<annotation_type> m_annotation_list;
  std::shared_ptr<partition_info> m_partition;
  std::shared_ptr<target_info> m_target;
  std::shared_ptr<aie_row_topology_info> m_aie_row_topology;
  const std::string m_target_type;
  const file_artifact* m_artifacts;
  std::map<int, std::pair<std::string, std::string>> m_preempt_labels;  // group -> (save_label, restore_label)
  std::map<int, std::vector<std::string>> m_preempt_hintmaps;  // group -> vector of hintmap_labels (multiple PREEMPT opcodes per group)
  std::map<std::string, std::pair<std::string, std::string>> m_hintmap_labels;  // hintmap_label -> (save_label, restore_label)
  std::set<int> m_preempt_without_hintmap;  // groups that have PREEMPT opcodes without hintmaps

  // One unique scratchpad region: all hintmap labels that share the same scratchbase+size
  struct hintmap_group_entry {
    std::vector<std::string>         hintmaps;    // hintmap labels sharing this scratchpad
    std::pair<std::string,std::string> labels;    // shared save/restore label pair
    uint64_t                         scratchbase;
    uint64_t                         size;
  };

  // Group hintmap labels by (scratchbase, size) and assign unique save/restore labels.
  std::vector<hintmap_group_entry> build_hintmap_groups(int group,
                                                        const std::vector<std::string>& hintmap_labels,
                                                        int group_index);

  // Register scratchpad and inject patched save/restore asm for one hintmap group.
  void inject_hintmap_save_restore(int col,
                                   const std::string& save_file,
                                   const std::string& restore_file,
                                   const std::vector<uint8_t>& save_data,
                                   const std::vector<uint8_t>& restore_data,
                                   const std::pair<std::string,std::string>& template_labels,
                                   const hintmap_group_entry& grp,
                                   const std::vector<std::string>& save_bd,
                                   const std::vector<std::string>& restore_bd,
                                   const std::vector<std::string>& save_membd,
                                   const std::vector<std::string>& restore_membd);

  // Walk column col and update PREEMPT opcode args to reflect shared labels.
  void update_preempt_opcodes(int col);

  // Inject default (no-hintmap) save/restore asm into column col.
  void inject_default_save_restore(int col,
                                   const std::string& save_file,
                                   const std::string& restore_file,
                                   const std::vector<uint8_t>& save_data,
                                   const std::vector<uint8_t>& restore_data,
                                   const std::vector<std::string>& save_bd,
                                   const std::vector<std::string>& restore_bd,
                                   const std::vector<std::string>& save_membd,
                                   const std::vector<std::string>& restore_membd);

  // Orchestrate all PREEMPT injections (hintmap + default) for one group.
  void process_preempt_group(int group,
                             int group_index,
                             const std::string& save_file,
                             const std::string& restore_file,
                             const std::vector<uint8_t>& save_data,
                             const std::vector<uint8_t>& restore_data,
                             const std::vector<std::string>& save_bd,
                             const std::vector<std::string>& restore_bd,
                             const std::vector<std::string>& save_membd,
                             const std::vector<std::string>& restore_membd);

public:
  asm_parser(const std::vector<char>& data, const std::vector<std::string>& include_list, const std::string& target_type, const file_artifact* artifacts = nullptr)
    :m_data(data),  m_include_list(include_list), m_target_type(target_type), m_artifacts(artifacts)
  {
    set_data_state(false);
    m_current_col = -1;
    m_partition = std::make_shared<partition_info>(DEFAULT_COLUMN, 0);
    m_target = std::make_shared<target_info>();
    m_aie_row_topology = std::make_shared<aie_row_topology_info>();
  }
  void set_data_state(bool state) { isdatastack.push(state); }

  void pop_data_state() { isdatastack.pop();}

  bool get_data_state() const { return isdatastack.top();}

  void set_annotation_state() { annotation_state = true; }

  void reset_annotation_state() { annotation_state = false; }

  bool get_annotation_state() { return annotation_state; }

  const std::string& get_target_type() const { return m_target_type; }

  bool is_multi_column_mode() const { return m_preempt_labels.size() > 1; }

  // Record preempt label for current group (called when PREEMPT opcode is hit)
  // Label naming: save_N / restore_N where N = index (group/2 + 1)
  // group 0 -> save_1, group 2 -> save_2, group 4 -> save_3
  void record_preempt_label(int group) {
    int index = group / 2 + 1;
    std::string save_label = "save_" + std::to_string(index);
    std::string restore_label = "restore_" + std::to_string(index);
    m_preempt_labels[group] = {save_label, restore_label};
    // Increment preempt count for the current col
    if (m_current_col >= 0) {
      auto it = m_col.find(m_current_col);
      it->second.increment_preempt_count();
    }
  }

  // Generate unique save/restore labels for a hintmap
  // Returns (save_label, restore_label) pair unique to this hintmap
  // Note: Labels are assigned during finalize_preempt() based on (scratchbase, size)
  // This function returns temporary labels during parsing, which will be updated later
  std::pair<std::string, std::string> get_hintmap_save_restore_labels(const std::string& hintmap_label, int group) {
    // Check if we already have labels for this hintmap (from finalize_preempt)
    if (m_hintmap_labels.find(hintmap_label) != m_hintmap_labels.end()) {
      return m_hintmap_labels[hintmap_label];
    }

    // Generate temporary labels based on group and hintmap index
    // These will be updated in finalize_preempt() based on (scratchbase, size)
    int hintmap_index = m_preempt_hintmaps[group].size();
    int group_index = group / 2 + 1;
    std::string save_label = "save_" + std::to_string(group_index) + "_" + std::to_string(hintmap_index);
    std::string restore_label = "restore_" + std::to_string(group_index) + "_" + std::to_string(hintmap_index);

    std::pair<std::string, std::string> labels = {save_label, restore_label};
    // Don't store in m_hintmap_labels yet - will be updated in finalize_preempt() based on (scratchbase, size)
    return labels;
  }

  const std::map<int, std::pair<std::string, std::string>>& get_preempt_labels() const { return m_preempt_labels; }

  // Verify that all columns have the same PREEMPT opcode count as column 0.
  // This ensures uniform preemption behavior across all controllers in the partition.
  // Returns a tuple: {success, expected_count, mismatched_col, mismatched_count}
  //   - success: true if all columns match col 0's count, false otherwise
  //   - expected_count: the preempt count from column 0 (reference)
  //   - mismatched_col: column number that has a different count (if success=false)
  //   - mismatched_count: the actual count in the mismatched column (if success=false)
  std::tuple<bool, uint32_t, int, uint32_t> verify_preempt_count() const {
    // Get column 0 as the reference for expected preempt count
    auto it = m_col.find(0);
    if (it == m_col.end())
      throw error(error::error_code::internal_error,
                  "Controlcode for Column 0 not found for preempt count verification\n");

    uint32_t expected_count = it->second.get_preempt_count();

    // Compare all other columns against column 0
    for (const auto& [col, data] : m_col) {
      if (col == 0) continue;  // Skip reference column
      uint32_t count = data.get_preempt_count();
      if (count != expected_count) {
        return {false, expected_count, col, count};  // Mismatch found
      }
    }
    return {true, expected_count, 0, 0};  // All columns match
  }

  // Check if any column in the control code contains PREEMPT opcodes
  bool has_preempt() const {
    for (const auto& [col, data] : m_col) {
      if (data.get_preempt_count() > 0)
        return true;
    }
    return false;
  }

  std::pair<std::vector<uint8_t>, std::vector<uint8_t>> get_preempt_save_restore(uint32_t key) const;
  std::pair<std::vector<std::string>, std::vector<std::string>> get_preempt_save_restore_bd(uint32_t key) const;
  std::pair<std::vector<std::string>, std::vector<std::string>> get_preempt_save_restore_shimbd(uint32_t key) const;
  std::pair<std::vector<std::string>, std::vector<std::string>> get_preempt_save_restore_membd(uint32_t key) const;

  void finalize_preempt();  // Called after parsing to inject actual save/restore code

  // Parse hintmap data and calculate scratchbase and size.
  // label_context: the label scope to search within (e.g. "default", "default:pdi").
  // Find which label scope inside column 'group' contains 'hintmap_label'.
  // If search_context is non-empty only that scope is checked.  Throws when not found.
  std::string find_hintmap_context(int group,
                                   const std::string& search_context,
                                   const std::string& hintmap_label);

  // When empty the label is resolved by scanning all scopes (only used for the
  // no-hintmap default case where hintmap_label is also empty).
  std::pair<uint64_t, uint64_t> parse_hintmap_and_calculate_scratchpad(int group,
                                                                         const std::string& label_context,
                                                                         const std::string& hintmap_label);

  // Replace save/restore labels in template code label/.endl label
  std::vector<char> replace_save_restore_labels(const std::vector<uint8_t>& template_data,
                                                 const std::string& old_save_label,
                                                 const std::string& old_restore_label,
                                                 const std::string& new_save_label,
                                                 const std::string& new_restore_label);
  void insert_annotation(int annotation_index);

  std::vector<annotation_type> get_annotations() { return std::move(m_annotation_list); }

  const std::vector<std::string>& get_include_list() const { return m_include_list; }
  std::string get_current_label() const { return m_current_label; }
  const file_artifact* get_artifacts() const { return m_artifacts;}

  std::string top_label() const
  {
    std::vector<std::string> labels = splitoption(m_current_label.c_str(), ':');
    if (labels.size() == 0)
      throw error(error::error_code::internal_error, "invalid current label:" + m_current_label);
    return labels[labels.size() - 1];
  }

  void pop_label()
  {
    size_t pos = m_current_label.rfind(':');
    if (pos != std::string::npos) {
        m_current_label =  m_current_label.substr(0, pos);
    }
  }

  void set_current_label(std::string& label) { m_current_label = label; }

  void insert_col_asmdata(std::shared_ptr<asm_data> data);

  std::vector<uint32_t> get_col_list();

  col_data& get_col_asmdata(uint32_t colnum);

  std::shared_ptr<const partition_info> get_partition_info() const { return std::const_pointer_cast<const partition_info>(m_partition); }

  void set_numcolumn(uint32_t val) { m_partition->set_numcolumn(val); }

  void set_numcore(uint32_t val) { m_partition->set_numcore(val); }

  void set_nummem(uint32_t val) { m_partition->set_nummem(val); }

  // Target info getters and setters
  std::shared_ptr<const target_info> get_target_info() const { return std::const_pointer_cast<const target_info>(m_target); }

  void set_target_arch(const std::string& arch) { m_target->set_arch(arch); }

  void set_target_sub_arch(const std::string& sub_arch) { m_target->set_sub_arch(sub_arch); }

  // AIE row topology info getters and setters
  std::shared_ptr<const aie_row_topology_info> get_aie_row_topology_info() const { return std::const_pointer_cast<const aie_row_topology_info>(m_aie_row_topology); }

  void set_num_south_shim(uint32_t val) { m_aie_row_topology->set_num_south_shim(val); }

  void set_num_memtile_row(uint32_t val) { m_aie_row_topology->set_num_memtile_row(val); }

  void set_num_coretile_row(uint32_t val) { m_aie_row_topology->set_num_coretile_row(val); }

  void set_num_north_shim(uint32_t val) { m_aie_row_topology->set_num_north_shim(val); }

  void set_aie_row_topology_is_set(bool val) { m_aie_row_topology->set_is_set(val); }

  void parse_lines();

  void parse_lines(const std::vector<char>& data, std::string& file);

  std::vector<char> get_asm_data(const std::string& name);
  void set_current_col(int col) { m_current_col = col;
    m_col[m_current_col] = col_data();
  }

  std::map<std::string, uint32_t>& getcollabelpageindex(int col) { return m_col[col].get_labelpageindex(); }

  std::vector<std::string> getLabelsforcol(uint32_t col)
  {
    std::vector<std::string> keys;
    for (const auto& pair : m_col[col].get_label_data()) {
        keys.push_back(pair.first);
    }
    return keys;
  }

  void insert_scratchpad(std::string& name, offset_type size, std::vector<char>& content);

  std::map<std::string, std::shared_ptr<scratchpad_info>>& getcolscratchpad(int col) { return m_col[col].get_scratchpads(); }

  bool operate_directive(const std::string& line)
  {
    smatch sm;
    const static regex DIRCETIVE_REGEX("^([.a-zA-Z0-9_]+)(?:[ \\t]+(.+))?$");
    regex_match(line, sm, DIRCETIVE_REGEX);
    if (sm.size() == 0)
      return false;

    if (directive_list.count(sm[1].str()) == 0)
      return false;

    directive_list[sm[1].str()]->operate(shared_from_this(), sm);
    return true;
  }
};



}
#endif // AIEBU_PREPROCESSOR_ASM_ASM_PARSER_H_
