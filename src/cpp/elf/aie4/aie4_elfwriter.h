// SPDX-License-Identifier: MIT
// Copyright (C) 2025, Advanced Micro Devices, Inc. All rights reserved.

#ifndef AIEBU_ELF_AIE4_ELF_WRITER_H_
#define AIEBU_ELF_AIE4_ELF_WRITER_H_

#include <aie2ps_elfwriter.h>

namespace aiebu {

class aie4_elf_writer: public aie2ps_elf_writer
{
public:
  aie4_elf_writer(): aie2ps_elf_writer()
  { }
};

}
#endif //AIEBU_ELF_AIE4_ELF_WRITER_H_
