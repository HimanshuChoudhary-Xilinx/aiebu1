// SPDX-License-Identifier: MIT
// Copyright (C) 2025 Advanced Micro Devices, Inc. All rights reserved.

#include <cstdint>
#include <boost/interprocess/streams/bufferstream.hpp>
#include <iterator>

#include <elfio/elfio.hpp>
#include <elfio/elfio_section.hpp>
#include "aiebu/aiebu_assembler.h"
#include "aiebu/aiebu_error.h"
#include "specification/aie2ps/isa.h"
#include "ops/ops.h"
#include "common/symbol.h"
#include "common/utils.h"
#include "analyzer/transform_manager.h"

namespace aiebu {

struct apply_offset_57 {
  uint8_t opcode;
  uint8_t pad;
  uint16_t table_ptr;
  uint16_t num_entries;
  uint16_t offset;
};

transform_manager::
transform_manager(const std::vector<char>& elf_data)
{
  load_elf(elf_data);
  isa_op_map = m_isa_disassembler.get_isa_map();
}

void
transform_manager::
load_elf(const std::vector<char>& elf_data)
{
  if (elf_data.empty())
    throw error(error::error_code::invalid_input, "Input buffer is empty");

  boost::interprocess::ibufferstream istr(elf_data.data(), elf_data.size());

  if (!m_elfio.load(istr))
    throw error(error::error_code::invalid_input, "Failed to load ELF from buffer\n");

  // only aie2ps/aie4 legacy elf and group elf supported
  auto os_abi = m_elfio.get_os_abi();
  if (os_abi != Elf_Amd_Aie2ps && os_abi != Elf_Amd_Aie2ps_group)
    throw error(error::error_code::invalid_input, "Only aie2ps/aie4 elf supported\n");
}

uint32_t
transform_manager::
size(const isa_op_disasm& op) const
{
  uint32_t total_width = 2; // 1 opcode + 1 pad
  for (const auto& arg : op.get_args())
    total_width += (arg.get_width() / byte_to_bits);

  return total_width;
}

void
transform_manager::
modify_apply_offset_57(char* text_section_data, size_t text_section_size, uint32_t section_idx)
{
  for (size_t offset = ELF_SECTION_HEADER_SIZE; offset < text_section_size;) {
    uint8_t opcode = *reinterpret_cast<const uint8_t*>(text_section_data + offset);

    if (opcode == ALIGN_OPCODE) {
      ++offset;
      continue;
    }

    auto op_it = isa_op_map->find(opcode);
    if (op_it == isa_op_map->end())
      throw error(error::error_code::invalid_asm, "Unknown Opcode:" + std::to_string(opcode) + " at position " + std::to_string(offset) + "\n");

    if (opcode == OPCODE_APPLY_OFFSET_57) {
      auto code = reinterpret_cast<apply_offset_57*>(text_section_data + offset);
      auto key = get_key(code->table_ptr, section_idx);
      // if key found means its kernel arg else its .ctrlpkt-idx or control-code-idx
      auto lookup_it = xrt_idx_lookup.find(key);
      if (lookup_it != xrt_idx_lookup.end())
        code->offset = static_cast<uint16_t>(lookup_it->second * Num_32bit_Register); // its register offset
    }

    offset += size(op_it->second);
  }
}

void
transform_manager::
process_sections() {
  ELFIO::Elf_Half num = 0;
  for (const auto& section_ptr : m_elfio.sections) {
    const ELFIO::section* section = section_ptr.get();
    const auto& section_name = section->get_name();

    if (is_text_section(section_name) && section->get_type() == ELFIO::SHT_PROGBITS)
      modify_apply_offset_57(const_cast<char *>(section->get_data()), section->get_size(), num);

    ++num;
  }
}

std::pair<uint32_t, uint32_t>
transform_manager::
get_column_and_page(const std::string& name) const
{
  // section name can be
  // .ctrltext.<col>.<page> or .ctrldata.<col>.<page>
  // .ctrltext.<col>.<page>.<id> or .ctrldata.<col>.<page>.<id> - newer Elfs

  // Max expected tokens: prefix, col, page, id
  constexpr size_t col_token_id  = 1;
  constexpr size_t page_token_id = 2;

  std::vector<std::string> tokens;
  std::stringstream ss(name);
  std::string token;
  while (std::getline(ss, token, '.')) {
    if (!token.empty())
      tokens.emplace_back(std::move(token));
  }

  try {
    if (tokens.size() <= col_token_id)
      return {0, 0}; // Only prefix present

    if (tokens.size() == (col_token_id + 1))
      return {std::stoul(tokens[col_token_id]), 0}; // Only col present

    return {std::stoul(tokens[col_token_id]), std::stoul(tokens[page_token_id])};
  }
  catch (const std::exception&) {
    throw std::runtime_error("Invalid section name passed to parse col or page index\n");
  }
}

std::string
transform_manager::
get_grp_id_if_group_elf(const std::string& name) const
{
  // section name can be
  // .ctrltext.<col>.<page> or .ctrldata.<col>.<page>
  // .ctrltext.<col>.<page>.<id> or .ctrldata.<col>.<page>.<id> - newer Elfs

  // Max expected tokens: prefix, col, page, id
  constexpr size_t group_elf_token = 4;

  std::vector<std::string> tokens;
  std::stringstream ss(name);
  std::string token;
  while (std::getline(ss, token, '.')) {
    if (!token.empty())
      tokens.emplace_back(std::move(token));
  }

  try {
    if (tokens.size() == group_elf_token)
      return tokens[group_elf_token -1];
  }
  catch (const std::exception&) {
    throw std::runtime_error("Invalid section name passed to parse col or page index\n");
  }
  return "";
}

uint64_t
transform_manager::
read57(const uint32_t* bd_data_ptr) const
{
  uint64_t base_address =
    ((static_cast<uint64_t>(bd_data_ptr[8]) & 0x1FF) << 48) |                       // NOLINT
    ((static_cast<uint64_t>(bd_data_ptr[2]) & 0xFFFF) << 32) |                      // NOLINT
    bd_data_ptr[1];
  return base_address;
}

uint64_t
transform_manager::
read57_aie4(const uint32_t* bd_data_ptr) const
{
  uint64_t base_address =
    ((static_cast<uint64_t>(bd_data_ptr[0]) & 0x1FFFFFF) << 32) |                   // NOLINT
    bd_data_ptr[1];
  return base_address;
}

void
transform_manager::
write57(uint32_t* bd_data_ptr, uint64_t bd_offset)
{
  bd_data_ptr[1] = static_cast<uint32_t>(bd_offset & 0xFFFFFFFF);                           // NOLINT
  bd_data_ptr[2] = static_cast<uint32_t>((bd_data_ptr[2] & 0xFFFF0000) | ((bd_offset >> 32) & 0xFFFF)); // NOLINT
  bd_data_ptr[8] = static_cast<uint32_t>((bd_data_ptr[8] & 0xFFFFFE00) | ((bd_offset >> 48) & 0x1FF));  // NOLINT
}

void
transform_manager::
write57_aie4(uint32_t* bd_data_ptr, uint64_t bd_offset)
{
  bd_data_ptr[1] = static_cast<uint32_t>(bd_offset & 0xFFFFFFFF);                           // NOLINT
  bd_data_ptr[0] = static_cast<uint32_t>((bd_data_ptr[0] & 0xFE000000) | ((bd_offset >> 32) & 0x1FFFFFF));// NOLINT
}

uint64_t
transform_manager::
ctrlpkt_read57(const uint32_t* bd_data_ptr) const
{
  uint64_t base_address =
    ((static_cast<uint64_t>(bd_data_ptr[3]) & 0xFFF) << 32) |                       // NOLINT
    ((static_cast<uint64_t>(bd_data_ptr[2])));

  return base_address;
}

void
transform_manager::
ctrlpkt_write57(uint32_t* bd_data_ptr, uint64_t bd_offset)
{
  bd_data_ptr[2] = static_cast<uint32_t>(bd_offset & 0xFFFFFFFC);                           // NOLINT
  bd_data_ptr[3] = static_cast<uint32_t>((bd_data_ptr[3] & 0xFFFF0000) | (bd_offset >> 32));            // NOLINT
}

uint64_t
transform_manager::
ctrlpkt_read57_aie4(const uint32_t* bd_data_ptr) const
{
  // bd_data_ptr is a pointer to the header of the control code
  uint64_t base_address = (((uint64_t)bd_data_ptr[1] & 0x1FFFFFF) << 32) | bd_data_ptr[2]; // NOLINT
  return base_address;
}

void
transform_manager::
ctrlpkt_write57_aie4(uint32_t* bd_data_ptr, uint64_t bd_offset)
{
  bd_data_ptr[2] = static_cast<uint32_t>(bd_offset & 0xFFFFFFFF);                                  // NOLINT
  bd_data_ptr[1] = static_cast<uint32_t>((bd_data_ptr[1] & 0xFE000000) | ((bd_offset >> 32) & 0x1FFFFFF));     // NOLINT
}

uint64_t
transform_manager::
get_controlcode_bd_offset(const std::string& section_name, uint32_t offset, symbol::patch_schema schema)
{
  auto [col, page] = get_column_and_page(section_name);
  auto id = get_grp_id_if_group_elf(section_name);
  auto ctrltext = m_elfio.sections[get_ctrltext_section_name(col, page, id)];
  auto ctrldata = m_elfio.sections[get_ctrldata_section_name(col, page, id)];
  if (!ctrltext || !ctrldata)
    throw error(error::error_code::internal_error, "ctrltext or ctrldata section for col:"
                + std::to_string(col) + " page:" + std::to_string(page) + " not found\n");

  if (offset < ctrltext->get_size())
    throw error(error::error_code::internal_error, "ctrltext size lesser than offset:"
                + std::to_string(offset) + "\n");

  offset -= ctrltext->get_size();
  offset += ELF_SECTION_HEADER_SIZE;

  const auto* bd_data_ptr = reinterpret_cast<const uint32_t*>(ctrldata->get_data() + offset);

  switch(schema) {
  case symbol::patch_schema::shim_dma_57:
    return read57(bd_data_ptr);
  case symbol::patch_schema::shim_dma_57_aie4:
    return read57_aie4(bd_data_ptr);
  default:
    throw error(error::error_code::internal_error, "Invalid schema found\n");
  }
}

void
transform_manager::
set_controlcode_bd_offset(const std::string& section_name, uint32_t offset, uint64_t bd_offset, symbol::patch_schema schema)
{
  auto [col, page] = get_column_and_page(section_name);
  auto id = get_grp_id_if_group_elf(section_name);
  auto ctrltext = m_elfio.sections[get_ctrltext_section_name(col, page, id)];
  auto ctrldata = m_elfio.sections[get_ctrldata_section_name(col, page, id)];
  if (!ctrltext || !ctrldata)
    throw error(error::error_code::internal_error, "ctrltext or ctrldata section for col:"
                + std::to_string(col) + " page:" + std::to_string(page) + " not found\n");
  if (offset < ctrltext->get_size())
    throw error(error::error_code::internal_error, "ctrldata size lesser than offset:"
                + std::to_string(offset) + "\n");

  offset -= ctrltext->get_size();
  offset += ELF_SECTION_HEADER_SIZE;
  if (offset > ctrldata->get_size())
    throw error(error::error_code::internal_error, "ctrltext size lesser than offset:"
                + std::to_string(offset) + "\n");

  auto* bd_data_ptr = reinterpret_cast<uint32_t*>(const_cast<char*>(ctrldata->get_data()) + offset);

  switch(schema) {
  case symbol::patch_schema::shim_dma_57:
    write57(bd_data_ptr, bd_offset);
    break;
  case symbol::patch_schema::shim_dma_57_aie4:
    write57_aie4(bd_data_ptr, bd_offset);
    break;
  default:
    throw error(error::error_code::internal_error, "Invalid schema found\n");
  }
}

uint64_t
transform_manager::
get_ctrlpkt_bd_offset(const std::string& section_name, uint32_t offset, symbol::patch_schema schema)
{
  auto ctrlpkt = m_elfio.sections[section_name];
  if (!ctrlpkt)
    throw error(error::error_code::internal_error, "ctrlpkt " + section_name + " not found\n");
  if (offset > ctrlpkt->get_size())
    throw error(error::error_code::internal_error, "ctrlpkt size lesser than offset:"
                + std::to_string(offset) + "\n");


  const auto* bd_data_ptr = reinterpret_cast<const uint32_t*>(ctrlpkt->get_data() + offset);

  switch(schema) {
  case symbol::patch_schema::control_packet_57:
    return ctrlpkt_read57(bd_data_ptr);
  case symbol::patch_schema::control_packet_57_aie4:
    return ctrlpkt_read57_aie4(bd_data_ptr);
  default:
    throw error(error::error_code::internal_error, "Invalid schema found\n");
  }
}

void
transform_manager::
set_ctrlpkt_bd_offset(const std::string& section_name, uint32_t offset, uint64_t bd_offset, symbol::patch_schema schema)
{
  auto ctrlpkt = m_elfio.sections[section_name];
  if (!ctrlpkt)
    throw error(error::error_code::internal_error, "ctrlpkt " + section_name + " not found\n");
  if (offset > ctrlpkt->get_size())
    throw error(error::error_code::internal_error, "ctrlpkt size lesser than offset:"
                + std::to_string(offset) + "\n");

  auto* bd_data_ptr = reinterpret_cast<uint32_t*>(const_cast<char*>(ctrlpkt->get_data()) + offset);

  switch(schema) {
  case symbol::patch_schema::control_packet_57:
    ctrlpkt_write57(bd_data_ptr, bd_offset);
    break;
  case symbol::patch_schema::control_packet_57_aie4:
    ctrlpkt_write57_aie4(bd_data_ptr, bd_offset);
    break;
  default:
    throw error(error::error_code::internal_error, "Invalid schema found\n");
  }
}

std::vector<arginfo>
transform_manager::
extract_rela_sections()
{
  auto dynsym = m_elfio.sections[".dynsym"];
  auto dynstr = m_elfio.sections[".dynstr"];
  auto dynsec = m_elfio.sections[".rela.dyn"];

  if (!dynsym || !dynstr || !dynsec)
    return {};

  const auto dynsym_size = dynsym->get_size();
  const auto dynstr_size = dynstr->get_size();
  const auto rela_count = dynsec->get_size() / sizeof(ELFIO::Elf32_Rela);

  std::vector<arginfo> entries;

  auto begin = reinterpret_cast<const ELFIO::Elf32_Rela*>(dynsec->get_data());
  auto end = begin + rela_count;

  for (auto rela = begin; rela != end; ++rela) {
    auto symidx = ELFIO::get_sym_and_type<ELFIO::Elf32_Rela>::get_r_sym(rela->r_info);
    auto type = ELFIO::get_sym_and_type<ELFIO::Elf32_Rela>::get_r_type(rela->r_info);

    auto dynsym_offset = symidx * sizeof(ELFIO::Elf32_Sym);
    if (dynsym_offset >= dynsym_size)
      throw error(error::error_code::internal_error, "Invalid symbol index " + std::to_string(symidx));
    auto sym = reinterpret_cast<const ELFIO::Elf32_Sym*>(dynsym->get_data() + dynsym_offset);

    auto dynstr_offset = sym->st_name;
    if (dynstr_offset >= dynstr_size)
      throw error(error::error_code::internal_error, "Invalid symbol name offset " + std::to_string(dynstr_offset));
    auto symname = dynstr->get_data() + dynstr_offset;

    // skip in case of control-code or ctrlpkt patches
    if (is_ctrlpkt_patch_name(symname) || is_controlcode_patch_name(symname))
      continue;

    auto patch_sec = m_elfio.sections[sym->st_shndx];
    if (!patch_sec)
      throw error(error::error_code::internal_error, "Invalid section index " + std::to_string(sym->st_shndx));

    auto patch_sec_name = patch_sec->get_name();
    auto offset = rela->r_offset;
    auto xrt_id = to_uinteger<uint32_t>(symname);
    auto schema = static_cast<symbol::patch_schema>(type);

    // bd can be read from ctrlcode or ctrlpkt section
    uint64_t bd_offset = 0;
    if (is_text_or_data_section_name(patch_sec_name))
      bd_offset = get_controlcode_bd_offset(patch_sec_name, offset, schema);
    else if (is_ctrlpkt_section_name(patch_sec_name))
      bd_offset = get_ctrlpkt_bd_offset(patch_sec_name, offset, schema);
    else
      throw error(error::error_code::internal_error, "Invalid section name " + patch_sec_name);

    entries.push_back({xrt_id, bd_offset + rela->r_addend});
  }

  return entries;
}

std::vector<char>
transform_manager::
update_rela_sections(const std::vector<arginfo>& entries) {
  xrt_idx_lookup.clear();
  auto dynsym = m_elfio.sections[".dynsym"];
  auto dynstr = m_elfio.sections[".dynstr"];
  auto dynsec = m_elfio.sections[".rela.dyn"];

  if (!dynsym || !dynstr || !dynsec)
    return {};

  const auto dynsym_size = dynsym->get_size();
  const auto dynstr_size = dynstr->get_size();
  const auto rela_count = dynsec->get_size() / sizeof(ELFIO::Elf32_Rela);

  // Copy and modify string table, first character is null
  std::string strtab_data(1, '\0');
  std::vector<char> dynsym_copy(dynsym->get_data(), dynsym->get_data() + dynsym_size);

  std::map<std::string, std::string> name_map;
  //hash to have same .dynsym to multiple .rela.dyn
  std::map<std::string, ELFIO::Elf_Word> hash;
  size_t num = 0;

  auto begin = reinterpret_cast<ELFIO::Elf32_Rela*>(const_cast<char*>(dynsec->get_data()));
  auto end = begin + rela_count;

  for (auto rela = begin; rela != end; ++rela) {
    auto symidx = ELFIO::get_sym_and_type<ELFIO::Elf32_Rela>::get_r_sym(rela->r_info);
    auto type = ELFIO::get_sym_and_type<ELFIO::Elf32_Rela>::get_r_type(rela->r_info);

    auto dynsym_offset = symidx * sizeof(ELFIO::Elf32_Sym);
    if (dynsym_offset >= dynsym_size)
      throw error(error::error_code::internal_error, "Invalid symbol index " + std::to_string(symidx));

    auto sym = reinterpret_cast<ELFIO::Elf32_Sym*>(const_cast<char*>(dynsym->get_data()) + dynsym_offset);
    auto sym_new = reinterpret_cast<ELFIO::Elf32_Sym*>(dynsym_copy.data() + dynsym_offset);

    auto dynstr_offset = sym->st_name;
    if (dynstr_offset >= dynstr_size)
      throw error(error::error_code::internal_error, "Invalid symbol name offset " + std::to_string(dynstr_offset));
    auto symname = dynstr->get_data() + dynstr_offset;

    // patching can be done to ctrlcode or ctrlpkt section
    auto patch_sec = m_elfio.sections[sym->st_shndx];
    if (!patch_sec)
      throw error(error::error_code::internal_error, "Invalid section index " + std::to_string(sym->st_shndx));

    const auto& patch_sec_name = patch_sec->get_name();
    auto offset = rela->r_offset;

    // in case of control-code-idx or .ctrlpkt-idx, we dont modify
    const bool is_special_patch = is_ctrlpkt_patch_name(symname) || is_controlcode_patch_name(symname);
    std::string name = is_special_patch ? symname : std::to_string(entries[num].xrt_idx);
    
    // Verify all instances of that xrt_id are modified consistently
    auto name_it = name_map.find(symname);
    if (name_it != name_map.end()) {
      if (name_it->second != name)
        throw error(error::error_code::invalid_input, "Invalid input xrt_id:" + std::string(symname) + " modified only at few places");
    } else {
      name_map[symname] = name;
    }

    // Check if we already have this key in hash
    ELFIO::Elf_Word new_offset;
    std::string key = patch_sec_name + "_" + name + "_" + std::to_string(sym->st_size);
    auto hash_it = hash.find(key);
    if (hash_it == hash.end()) {
      new_offset = strtab_data.size();
      strtab_data.append(name).push_back('\0');
      hash[key] = new_offset;
    } else {
      new_offset = hash_it->second;
    }

    sym_new->st_name = new_offset;

    // skip patching for symbol control-code-idx, as it doesnt change
    // skip patching for symbol .ctrlpkt-idx, as it doesnt change
    if (is_special_patch == true)
      continue;

    // Create lookup table to map xrt-id during modifying apply-offset-57 opcode
    auto lookup_key = get_key(offset, sym->st_shndx);
    if (xrt_idx_lookup.find(lookup_key) != xrt_idx_lookup.end())
      throw error(error::error_code::internal_error, "lookup_key (" + lookup_key + ") already found\n");

    xrt_idx_lookup[lookup_key] = entries[num].xrt_idx;

    // patching can be done to ctrlcode or ctrlpkt section
    auto schema = static_cast<symbol::patch_schema>(type);
    if (is_text_or_data_section_name(patch_sec_name))
      set_controlcode_bd_offset(patch_sec_name, offset, entries[num].bd_offset, schema);
    else if (is_ctrlpkt_section_name(patch_sec_name))
      set_ctrlpkt_bd_offset(patch_sec_name, offset, entries[num].bd_offset, schema);
    else
      throw error(error::error_code::internal_error, "Invalid section name " + patch_sec_name);
    rela->r_addend = 0;
    ++num;
  }

  // Replace old symstr with new
  dynstr->set_data(strtab_data);
  dynsym->set_data(dynsym_copy.data(), dynsym_copy.size());

  process_sections();

  // Save modified ELF
  std::stringstream stream;
  stream << std::noskipws;
  m_elfio.save(stream);

  return std::vector<char>(std::istream_iterator<char>(stream), std::istream_iterator<char>());
}
} // End of Namespace aiebu

