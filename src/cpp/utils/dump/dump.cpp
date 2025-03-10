// SPDX-License-Identifier: MIT
// Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

#include <boost/format.hpp>
#include <cxxopts.hpp>
#include <set>
#include <string>
#include <iostream>

namespace aiebu::utilities {

static const std::set<std::string> targets = { //NOLINT
    "aie2ps",
    "aie2asm",
    "aie2txn",
    "aie2dpu",
    "auto"
};

void main_helper(int argc, const char* const *argv,
                 const std::string & executable,
                 const std::string & description)
{
  bool bhelp = false;
  std::string target_name;
  std::vector<std::string> subcmd_options;
  cxxopts::Options global_options(executable, description);

  try {
    global_options
      .allow_unrecognised_options()
      .add_options()
      ("h,help", "show help message and exit", cxxopts::value<bool>()->default_value("false"))
      ("t,target", "supported targets aie2ps/aie2asm/aie2txn/aie2dpu", cxxopts::value<decltype(target_name)>()->default_value("auto"))
      ("x,all-headers", "display contents of all elf headers", cxxopts::value<bool>()->default_value("false"))
      ("d,disassemble", "display assembler contents of ctrltext sections", cxxopts::value<bool>()->default_value("false"));

    auto result = global_options.parse(argc, argv);

    subcmd_options = result.unmatched();

    if (result.count("help"))
      bhelp = result["help"].as<bool>();

    if (result.count("target"))
      target_name = result["target"].as<decltype(target_name)>();
    if (targets.find(target_name) == targets.end())
      throw cxxopts::exceptions::incorrect_argument_type(target_name);
  }
  catch (const cxxopts::exceptions::exception& e) {
    auto errMsg = boost::format("Error parsing options: %s\n") % e.what() ;
    throw std::runtime_error(errMsg.str());
  }

  if (bhelp)
    std::cout << global_options.help({"", executable}) << std::endl;
}

} //namespace aiebu::utilities

int main(int argc, char* argv[])
{
  const std::string executable = "aiebu-dump";
  // -- Program Description
  const std::string description = "AIEBU Dumping utils (aiebu-dump)";

  try {
    aiebu::utilities::main_helper(argc, argv, executable, description);
    return 0;
  } catch (const std::exception& e) {
    std::cout << e.what();
  }

  return 1;
}
