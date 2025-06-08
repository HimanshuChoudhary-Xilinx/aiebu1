// SPDX-License-Identifier: MIT
// Copyright (C) 2025, Advanced Micro Devices, Inc. All rights reserved.

#ifndef AIEBU_ELF_CONFIG_ELF_WRITER_H_
#define AIEBU_ELF_CONFIG_ELF_WRITER_H_

#include <elfwriter.h>

namespace aiebu {

class config_elf_writer: public elf_writer
{
  constexpr static unsigned char ob_abi = 0x46;
  constexpr static unsigned char version = 0x02;
  const std::string const_configuration = "configuration";
  const std::string xrt_configuration = ".note.xrt.configuration";
  const std::string const_kernel_signature = "kernel.signature";

public:
  config_elf_writer(): elf_writer(ob_abi, version)
  { }

  std::vector<char>
  process(std::vector<std::shared_ptr<writer>>& mwriter) override
  {
    process_common_helper(mwriter, "");
    //std::string uuid = mwriter[0].get_metadata("kernel.config.uuid");
    //if (!uuid.empty())
    //  add_note(NT_XRT_UUID, ".note.xrt.kernel.config.uuid", uuid);

    auto element = std::dynamic_pointer_cast<section_writer>(mwriter[0]);
    const std::string configuration = element->get_metadata(const_configuration);
    std::vector<char> configuration_vec(configuration.begin(), configuration.end());
    if (!configuration.empty())
      add_note(NT_XRT_PARTITION_SIZE, xrt_configuration, configuration_vec);

    std::string kernel_signature = element->get_metadata(const_kernel_signature);
    if (!kernel_signature.empty())
    {
      init_symtab();
      add_symtab(kernel_signature);
    }
    return finalize();
  }
};

}
#endif //AIEBU_ELF_CONFIG_ELF_WRITER_H_
