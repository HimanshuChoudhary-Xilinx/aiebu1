// SPDX-License-Identifier: MIT
// Copyright (C) 2024-2025, Advanced Micro Devices, Inc. All rights reserved.

#ifndef _AIEBU_ELF_AIE2PS_ELF_WRITER_H_
#define _AIEBU_ELF_AIE2PS_ELF_WRITER_H_

#include <elfwriter.h>

namespace aiebu {

class aie2ps_elf_writer: public elf_writer
{
  constexpr static unsigned char ob_abi = 0x40;
  constexpr static unsigned char version = 0x02;
public:
  aie2ps_elf_writer(): elf_writer(ob_abi, version)
  { }
};

class aie2ps_config_elf_writer: public elf_writer
{
  constexpr static unsigned char ob_abi = 0x40;
  constexpr static unsigned char version = 0x03;
  const std::string const_configuration = "configuration";
  const std::string xrt_configuration = ".note.xrt.configuration";
  const std::string const_kernel_signature = "kernel.signature";

public:
  aie2ps_config_elf_writer(): elf_writer(ob_abi, version)
  { }

  void
  add_group(std::string name, std::vector<uint32_t> member, int info_index)
  {
    // add section
    ELFIO::section* sec = m_elfio.sections.add(name);
    sec->set_type(ELFIO::SHT_GROUP);
    sec->set_flags(ELFIO::SHF_ALLOC);
    sec->set_addr_align(align);
    sec->set_info(info_index);
    sec->set_entry_size(4);

    if(member.size())
      sec->set_data(reinterpret_cast<const char*>(member.data()), static_cast<ELFIO::Elf_Word>(member.size()*4));

    const ELFIO::section* lsec = m_elfio.sections[".symtab"];
    sec->set_link(lsec->get_index());
  }

  std::vector<char>
  process(std::vector<std::shared_ptr<writer>>& mwriter) override
  {
    auto mconfig_writer = std::dynamic_pointer_cast<config_writer>(mwriter[0]);
    init_symtab();
    int index=0;
    for( auto& [kernel, instances] : mconfig_writer->get_kernel_map())
    {
       auto kernel_index = add_symtab(kernel);
       for(auto& [iname, instance] : instances)
       {
         auto instance_index = add_symtab_section(iname, kernel_index);
         std::vector<uint32_t> group_data = std::move(process_common_helper(instance, "."+std::to_string(index)));
         //group_data.push_front(1);
         group_data.insert(group_data.begin(), 1);
         add_group(".group."+ std::to_string(index), group_data, instance_index);
         index++;
       }
    }
    if (dstr_sec)
      add_dynamic_section_segment();
    std::vector<char> configuration_vec(4);
    auto col = mconfig_writer->get_numcolumn();
    std::memcpy(configuration_vec.data(), &col, sizeof(uint32_t));

    add_note(NT_XRT_PARTITION_SIZE, xrt_configuration, configuration_vec);
    return finalize();
  }
};
}
#endif //_AIEBU_ELF_AIE2PS_ELF_WRITER_H_
