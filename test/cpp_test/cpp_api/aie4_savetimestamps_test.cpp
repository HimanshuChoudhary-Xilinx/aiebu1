// SPDX-License-Identifier: MIT
// Copyright (C) 2025 Advanced Micro Devices, Inc.

// Sample test for aiebu_assembler::get_save_timestamps()
//
// Two modes, selected by the first argument:
//
//   config <path>   — Assemble a config ELF (aie4_config, config.json driven)
//                     and call get_save_timestamps("DPU").
//                     The directory must contain:
//                       config.json, test.asm, aie_runtime_control.asm,
//                       pdi.asm, aie_asm_elfs.asm, aie_asm_enable.asm,
//                       aie_asm_init.asm
//
//   target <path>   — Assemble a standalone target ELF (asm_aie4, single ASM
//                     buffer) and call get_save_timestamps("") (empty kernel).
//                     The directory must contain test.asm (and its includes).

#include "aiebu/aiebu_assembler.h"
#include "aiebu/aiebu_error.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

static void read_file(const std::string& path, std::vector<char>& buf)
{
  if (!std::filesystem::exists(path))
    throw std::runtime_error("file not found: " + path);
  std::ifstream f(path, std::ios::binary);
  auto sz = std::filesystem::file_size(path);
  buf.resize(sz);
  f.read(buf.data(), static_cast<std::streamsize>(sz));
}

static void print_results(const aiebu::aiebu_assembler::savetimestamp_tbl& tbl,
                          const std::string& label)
{
  const auto& locs = tbl.get();

  if (locs.empty()) {
    std::cout << "No save_timestamps opcodes found" << (label.empty() ? "" : " for " + label) << "\n";
    return;
  }

  std::cout << "save_timestamps locations" << (label.empty() ? "" : " for " + label) << ":\n";
  for (const auto& loc : locs) {
    if (!loc.inst_name.empty())
      std::cout << "  instance: " << loc.inst_name << "\n";
    for (const auto& ts : loc.ts_line_info) {
      std::cout << "    col=" << ts.col << "\n";
      for (const auto& [linenum, filename] : ts.entries)
        std::cout << "      line=" << linenum << "  file=" << filename << "\n";
    }
  }
}

// ---------------------------------------------------------------------------
// config mode: aie4_config assembled from config.json + file search paths
// ---------------------------------------------------------------------------
static int test_config(const std::string& dir)
{
  std::vector<char> config_json;
  read_file(dir + "/config.json", config_json);

  // No "disabledump" flag → .dump section is included in the ELF, allowing
  // get_save_timestamps() to read timestamp locations from it.
  aiebu::aiebu_assembler as(
      aiebu::aiebu_assembler::buffer_type::aie4_config,
      {},
      {} /* no flags */,
      {dir},
      config_json);

  auto tbl = as.get_save_timestamps("DPU");
  if (tbl.get().empty()) {
    std::cerr << "FAIL: no save_timestamps found for kernel DPU\n";
    return 1;
  }
  print_results(tbl, "kernel DPU");
  return 0;
}

// ---------------------------------------------------------------------------
// target mode: standalone asm_aie4 ELF from a single ASM buffer
// ---------------------------------------------------------------------------
static int test_target(const std::string& dir)
{
  std::vector<char> asm_buf;
  read_file(dir + "/test.asm", asm_buf);

  // No "disabledump" flag → .dump section is written.
  // libpaths contains the test directory so that .include directives inside
  // test.asm resolve correctly (aie_runtime_control.asm, pdi.asm, etc.).
  aiebu::aiebu_assembler as(
      aiebu::aiebu_assembler::buffer_type::asm_aie4,
      asm_buf,
      std::vector<std::string>{} /* no flags */,
      std::vector<std::string>{dir} /* search path for .include */);

  // Empty kernel name → non-config ELF path: scans .dump section directly
  // without group-based kernel:instance filtering.
  auto tbl = as.get_save_timestamps("");
  if (tbl.get().empty()) {
    std::cerr << "FAIL: no save_timestamps found\n";
    return 1;
  }
  print_results(tbl, "");
  return 0;
}

int main(int argc, char** argv)
{
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <config|target> <testcase_path>\n";
    return 1;
  }

  const std::string mode = argv[1];
  const std::string dir  = argv[2];

  try {
    if (mode == "config")
      return test_config(dir);
    if (mode == "target")
      return test_target(dir);

    std::cerr << "Unknown mode '" << mode << "'. Use 'config' or 'target'.\n";
    return 1;
  }
  catch (const aiebu::error& ex) {
    std::cerr << "aiebu error: " << ex.what() << " (code=" << ex.get_code() << ")\n";
    return 1;
  }
  catch (const std::exception& ex) {
    std::cerr << "error: " << ex.what() << "\n";
    return 1;
  }
}
