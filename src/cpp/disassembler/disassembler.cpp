// SPDX-License-Identifier: MIT
// Copyright (C) 2025-2026, Advanced Micro Devices, Inc. All rights reserved.
#include "disassembler/disassembler.h"
#include "elf/aie_elf_constants.h"
#include "preprocessor/asm/hintmap_bitset.h"
#include <array>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <set>

namespace aiebu {

// Part of this code are generated using Cursor.
// ELF and Binary Format Constants
static constexpr size_t elf_section_header_padding = 16;  // ELF-specific header padding
static constexpr size_t page_size = 8192;                   // Binary page size (8KB)
static constexpr size_t page_header_size = 16;              // Page header size in bytes

// Page Header Field Offsets
static constexpr size_t page_header_magic_byte_0 = 0;       // First magic byte offset
static constexpr size_t page_header_magic_byte_1 = 1;       // Second magic byte offset
static constexpr size_t page_header_cur_len_low = 8;        // Current page length low byte offset
static constexpr size_t page_header_cur_len_high = 9;       // Current page length high byte offset
static constexpr size_t page_header_in_order_len_low = 10;  // In-order page length low byte offset
static constexpr size_t page_header_in_order_len_high = 11; // In-order page length high byte offset

// Magic Values
static constexpr uint8_t page_header_magic = 0xFF;          // Page header magic byte value
static constexpr uint8_t align_opcode = 0xA5;               // .align pseudo-instruction opcode
static constexpr uint8_t eof_opcode = 0xFF;                 // End-of-file opcode
static constexpr uint8_t zero_padding = 0x00;               // Zero padding byte

// Opcode Sizes
static constexpr size_t eof_size = 4;                       // EOF instruction size in bytes
static constexpr size_t align_4 = 4;                        // 4-byte alignment
static constexpr size_t align_16 = 16;                      // 16-byte alignment

// Section Name Lengths
static constexpr size_t ctrltext_string_length = 9;        // Length of ".ctrltext"
static constexpr size_t ctrldata_string_length = 9;        // Length of ".ctrldata"

// Bit Shift Constants
static constexpr size_t byte_shift = 8;                    // Bit shift for byte-to-word conversion

// Base class constructor
asm_disassembler::asm_disassembler(std::ostream& output_stream)
    : m_asm_writer(output_stream), m_buffer_type(aiebu_assembler::buffer_type::unspecified) {
    isa_op_map = isa_disasm.get_isa_map();
}

// Create architecture-specific disassembler state based on buffer_type
std::shared_ptr<disassembler_state> asm_disassembler::create_disassembler_state() const {
    switch (m_buffer_type) {
        case aiebu_assembler::buffer_type::elf_aie4:
        case aiebu_assembler::buffer_type::blob_aie4:
            return std::make_shared<disassembler_state_aie4>();
        default:
            // Default to aie2ps for aie2ps, aie2, aie2p, or unknown architectures
            return std::make_shared<disassembler_state_aie2ps>();
    }
}

// Common helper to write text section header
// Use .section directive so assembler switches to text mode after EOF
void asm_disassembler::add_text_sec_comment() {
    m_asm_writer.write_directive("");
    m_asm_writer.write_directive(".section .ctrltext");
    m_asm_writer.write_directive("");
}

// Common helper to write data section header
// Use .section directive for consistency (assembler switches to data mode after EOF anyway)
void asm_disassembler::add_data_sec_comment() {
    m_asm_writer.write_directive("");
    m_asm_writer.write_directive(".section .ctrldata");
    m_asm_writer.write_directive("");
}

// Common text block processing (used by both ELF and binary disassemblers)
void asm_disassembler::process_text_block(const char* data, size_t start_offset, size_t end_offset,
                                         std::shared_ptr<disassembler_state> state) {
    for (size_t offset = start_offset; offset < end_offset;) {
        uint8_t opcode = *reinterpret_cast<const uint8_t*>(data + offset);

        // Handle alignment padding
        if (opcode == align_opcode) {
            state->increment_address(1);
            ++offset;
            continue;
        }

        // Look up opcode in ISA map
        auto op_it = isa_op_map->find(opcode);
        if (op_it == isa_op_map->end()) {
            std::ostringstream err;
            err << "Unknown Opcode: 0x" << std::hex << static_cast<int>(opcode)
                << " at offset " << std::dec << offset << "\n";
            throw error(error::error_code::invalid_asm, err.str());
        }

        // Deserialize and consume bytes
        auto deserializer = op_it->second.create_deserializer();
        size_t consumed = deserializer->deserialize(m_asm_writer, state, data + offset);
        offset += consumed;
    }
}

// Internal helper: process a data block using an explicit writer (allows silent processing)
static void process_data_block_impl(const char* data, size_t size,
                                    std::shared_ptr<disassembler_state> state,
                                    asm_writer& writer) {
    isa_op_disasm dummy_isa_op("dummy", 0, std::vector<opArg>{});
    bool align_4_written = false;

    for (size_t offset = 0; offset < size;) {
        uint8_t opcode = *reinterpret_cast<const uint8_t*>(data + offset);
        const auto& label_map = state->get_labels();
        const auto& local_ptr_map = state->get_local_ptrs();

        // Check for UC_DMA_BD at label positions
        if (label_map.find(state->get_address()) != label_map.end()) {
            ucDmaBd_op_deserializer deserializer(&dummy_isa_op);
            size_t consumed = deserializer.deserialize(writer, state, data + offset);
            offset += consumed;
        }
        // Check for .long at local pointer positions
        else if (local_ptr_map.find(state->get_address()) != local_ptr_map.end()) {
            if (!align_4_written) {
                writer.write_directive("");
                writer.write_directive("  .align             " + std::to_string(align_4));
                align_4_written = true;
            }
            long_op_deserializer deserializer(&dummy_isa_op);
            size_t consumed = deserializer.deserialize(writer, state, data + offset);
            offset += consumed;
        }
        // Handle padding bytes
        else if (opcode == align_opcode || opcode == zero_padding) {
            state->increment_address(1);
            ++offset;
        }
        else {
            // Unknown byte in data region — emit as a raw 4-byte .long so the
            // disassembly does not lose information even when labels are missing.
            if (!align_4_written) {
                writer.write_directive("");
                writer.write_directive("  .align             " + std::to_string(align_4));
                align_4_written = true;
            }
            // Consume up to 4 bytes as one .long word (pad with zero if near end).
            uint32_t word = 0;
            size_t avail = size - offset;
            size_t take = (avail >= 4) ? 4 : avail;
            std::memcpy(&word, data + offset, take);
            std::ostringstream hex;
            hex << "  .long              0x" << std::hex << std::setw(8)
                << std::setfill('0') << word;
            writer.write_directive(hex.str());
            state->increment_address(static_cast<uint32_t>(take));
            offset += take;
        }
    }
}

// Common data block processing (used by both ELF and binary disassemblers)
void asm_disassembler::process_data_block(const char* data, size_t size,
                                         std::shared_ptr<disassembler_state> state) {
    process_data_block_impl(data, size, state, m_asm_writer);
}

// Process data block with an explicit writer (used for silent save page processing)
void elf_asm_disassembler::process_data_block_with_writer(const char* data, size_t size,
                                                          std::shared_ptr<disassembler_state> state,
                                                          asm_writer& writer) {
    process_data_block_impl(data, size, state, writer);
}

// ELF disassembler constructor
elf_asm_disassembler::elf_asm_disassembler(const std::string& input_elf_path, std::ostream& output_stream,
                                          aiebu_assembler::buffer_type buffer_type)
    : asm_disassembler(output_stream) {
    if (!m_elf_reader.load(input_elf_path)) {
        throw error(error::error_code::invalid_elf, "Failed to load ELF:" + input_elf_path + "\n");
    }
    m_buffer_type = buffer_type;
}

// ELF disassembler constructor from input stream
elf_asm_disassembler::elf_asm_disassembler(std::istream& input_stream, std::ostream& output_stream,
                                          aiebu_assembler::buffer_type buffer_type)
    : asm_disassembler(output_stream) {
    if (!m_elf_reader.load(input_stream)) {
        throw error(error::error_code::invalid_elf, "Failed to load ELF from input stream\n");
    }
    m_buffer_type = buffer_type;
}

// ELF disassembler run method
void elf_asm_disassembler::run() {
    process_sections();
}

// Binary disassembler constructor
bin_asm_disassembler::bin_asm_disassembler(const std::vector<char>& binary_data,
                                          std::ostream& output_stream,
                                          aiebu_assembler::buffer_type buffer_type)
    : asm_disassembler(output_stream), m_binary_data(binary_data) {
    m_buffer_type = buffer_type;

    // Output target architecture information for binary files
    std::string arch_name = (buffer_type == aiebu_assembler::buffer_type::blob_aie4) ? "aie4" : "aie2ps";
    m_asm_writer.write_directive("; Target Architecture: " + arch_name);
}

// Binary disassembler run method
void bin_asm_disassembler::run() {
    process_binary();
}

// Helper function to read page header fields from a text section
// Returns in_order_page_len (0 if this is the last page in the current label scope)
static uint16_t get_in_order_page_len(const ELFIO::section* section) {
    if (section->get_size() < elf_section_header_padding) {
        return 0;  // Section too small to have a page header
    }
    const char* data = section->get_data();
    // Verify magic bytes
    if (static_cast<uint8_t>(data[page_header_magic_byte_0]) != page_header_magic ||
        static_cast<uint8_t>(data[page_header_magic_byte_1]) != page_header_magic) {
        return 0;  // Not a valid page header
    }
    // Read in_order_page_len (bytes 10-11, little-endian)
    return static_cast<uint8_t>(data[page_header_in_order_len_low]) |
           (static_cast<uint8_t>(data[page_header_in_order_len_high]) << byte_shift);
}

void elf_asm_disassembler::process_sections() {
    auto state = create_disassembler_state();
    // Pre-scan using a separate state so label counters don't interfere with the main pass.
    // Prescan collects save page slots; .dump.N sections (if present) provide exact chunk
    // ranges for hintmap reconstruction.  The dump section is optional — if absent all
    // hintmaps emit zero words (NOP).
    {
        auto prescan_state = create_disassembler_state();
        prescan_hintmap_slots(prescan_state);
        state->import_valid_hintmap_labels(prescan_state->get_valid_hintmap_label_names());
        state->import_hintmap_chunk_ranges(prescan_state->get_hintmap_chunk_ranges());
    }

    // Count unique columns to determine partition size
    std::set<int> columns;
    int first_column = 0;
    for (const auto& section_ptr : m_elf_reader.sections) {
        const ELFIO::section* section = section_ptr.get();
        const std::string section_name = section->get_name();
        if (section->get_type() != ELFIO::SHT_PROGBITS)
            continue;
        if (is_text_section(section_name)) {
            // Parse column from section name like ".ctrltext.0.1" -> column 0
            size_t first_dot = section_name.find('.', 1);
            if (first_dot != std::string::npos) {
                size_t second_dot = section_name.find('.', first_dot + 1);
                if (second_dot != std::string::npos) {
                    std::string col_str = section_name.substr(first_dot + 1, second_dot - first_dot - 1);
                    try {
                        int col = std::stoi(col_str);
                        if (columns.empty()) first_column = col;
                        columns.insert(col);
                    } catch (...) { /* ignore parse errors */ }
                }
            }
        }
    }

    // Emit .target based on ELF OS/ABI and ABI version.
    // Only for abi_version >= 0x20 (versions that carry an explicit .target).
    {
        const unsigned char av    = m_elf_reader.get_abi_version();
        const unsigned char osabi = m_elf_reader.get_os_abi();
        std::string target;
        if (av >= elf_version_config_v1) {
            if      (osabi == osabi_aie4a)                              target = "aie4a";
            else if (osabi == osabi_aie4z)                              target = "aie4z";
            else if (osabi == osabi_aie4 || osabi == osabi_aie2ps_group) target = "aie4";
            else if (osabi == osabi_aie2p)                              target = "aie2p";
            else if (osabi == osabi_aie2ps)                             target = "aie2ps";
        }
        if (!target.empty())
            m_asm_writer.write_directive(".target\t " + target);
    }

    // Emit partition directive (number of columns)
    if (!columns.empty()) {
        m_asm_writer.write_partition(std::to_string(columns.size()) + "column");
    }

    // Emit initial attach_to_group
    m_asm_writer.write_attach_to_group(first_column);

    int current_column = first_column;
    int page_counter = 0;  // Track page number for unique labels
    std::string current_page_label;  // Track current page label for .endl
    uint16_t prev_in_order_page_len = 0;  // Track previous page's in_order_page_len

    for (const auto& section_ptr : m_elf_reader.sections) {
        const ELFIO::section* section = section_ptr.get();
        const std::string section_name = section->get_name();
        if (section->get_type() != ELFIO::SHT_PROGBITS)
            continue;

        // Check for column change in text sections
        if (is_text_section(section_name)) {
            size_t first_dot = section_name.find('.', 1);
            if (first_dot != std::string::npos) {
                size_t second_dot = section_name.find('.', first_dot + 1);
                if (second_dot != std::string::npos) {
                    std::string col_str = section_name.substr(first_dot + 1, second_dot - first_dot - 1);
                    try {
                        int section_col = std::stoi(col_str);
                        if (section_col != current_column) {
                            m_asm_writer.write_attach_to_group(section_col);
                            current_column = section_col;
                            // Page-index → slot mappings are per-section; clear them so
                            // a new column's page_idx values don't collide with prior ones.
                            state->reset_section_page_mappings();
                        }
                    } catch (...) { /* ignore parse errors */ }
                }
            }
        }

        print_section_info(section);
        if (is_text_section(section_name)) {
            // Note: add_text_sec_comment() is called inside process_text_section()
            // for merged-page sections (once per page).  For single-page sections
            // we emit it here so label/OOO logic below can still interleave correctly.
            const bool sec_is_merged =
                (m_elf_reader.get_abi_version() >= elf_version_config);  // 0x21+ = merged-page
            if (!sec_is_merged)
                add_text_sec_comment();

            // Read the current section's in_order_page_len for later use
            uint16_t current_in_order_len = get_in_order_page_len(section);

            // OOO label / page-scope logic only applies to the separate-page (legacy) ELF
            // format. In merged-page format (abi_version >= 0x21) all pages within a column
            // live inside one ELF section and are handled entirely inside process_text_section().
            if (!sec_is_merged && page_counter > 0 && prev_in_order_page_len == 0) {
                if (state->has_pending_ooo_labels()) {
                    // OOO labels (from load_pdi, etc.) establish new page scope
                    // Get and consume the OOO label, strip '@' prefix for .endl usage
                    std::string ooo_label = state->get_next_ooo_label();
                    if (!ooo_label.empty() && ooo_label.front() == '@')
                        current_page_label = ooo_label.substr(1);
                    else
                        current_page_label = ooo_label;
                    // Write the label (with @ prefix for ASM format)
                    m_asm_writer.write_label(ooo_label);
                } else {
                    // Create a unique page label with padded number for correct sorting
                    // Use format "zpage_NNNN" to sort AFTER "label0" (OOO target labels)
                    std::ostringstream oss;
                    oss << "zpage_" << std::setw(4) << std::setfill('0') << page_counter;
                    current_page_label = oss.str();
                    m_asm_writer.write_label(current_page_label);
                }
            }

            process_text_section(section, state);
            prev_in_order_page_len = current_in_order_len;  // Update for next iteration
            page_counter++;
        }
        if (is_data_section(section_name)) {
            add_data_sec_comment();
            process_data_section(section, state);
            state->reset();
            // Emit .endl to close the current page label scope when this scope ends
            // The scope ends when in_order_page_len was 0 (no continuation to next page)
            if (!current_page_label.empty() && prev_in_order_page_len == 0) {
                m_asm_writer.write_directive(".endl " + current_page_label);
                current_page_label.clear();
            }
        }
    }
}

void elf_asm_disassembler::print_section_info(const ELFIO::section* section) {
    std::string flags;
    if (section->get_flags() & ELFIO::SHF_ALLOC)
        flags += "a";
    if (section->get_flags() & ELFIO::SHF_WRITE)
        flags += "w";
    if (section->get_flags() & ELFIO::SHF_EXECINSTR)
        flags += "x";
    m_asm_writer.write_directive("");
    if (is_data_section(section->get_name()))
        m_asm_writer.write_directive("  .align             " + std::to_string(section->get_addr_align()));
}

void elf_asm_disassembler::process_text_section(const ELFIO::section* section, std::shared_ptr<disassembler_state> state) {
    const char* section_data = section->get_data();
    size_t section_size = section->get_size();

    // ABI version determines page layout:
    //   0x02 (elf_version_legacy)         = separate-page: one ctrltext section per page,
    //                                       ctrldata in a separate section.
    //   0x03 (elf_version_legacy_config)  = separate-page
    //   0x10 (elf_version_aie2p_config)   = separate-page
    //   0x20 (elf_version_config_v1)      = separate-page
    //   0x21 (elf_version_config)         = merged-page: ctrltext section is N×8KB pages,
    //                                       each page has header + code + EOF + ctrldata.
    const unsigned char abi_version = m_elf_reader.get_abi_version();
    const bool is_merged_pages = (abi_version >= elf_version_config);

    if (is_merged_pages) {
        // Process each 8 KB page independently.
        // Layout per page:
        //   [page_start .. page_start+16)  — page header (or section header for page 0)
        //   [page_start+16 .. eof_end)     — opcodes (text), terminated by 4-byte EOF
        //   [eof_end .. page_start+8192)   — data (ctrldata) for this page
        //
        // The page header bytes 10-11 (in_order_page_len): non-zero means the next page
        // continues in the same label scope.  A label opened by a load_pdi target page
        // must stay open across all consecutive in-order pages; .endl is only emitted
        // after the last page whose in_order_page_len == 0.
        std::string open_label;  // label scope currently open (empty = no open scope)

        for (size_t page_start = 0; page_start < section_size; page_start += page_size) {
            const size_t code_start = page_start + elf_section_header_padding;
            const size_t page_end   = page_start + page_size;

            // Read in_order_page_len from page header (bytes 10-11, little-endian).
            uint16_t page_in_order_len = 0;
            if (page_start + page_header_in_order_len_high < section_size) {
                page_in_order_len =
                    static_cast<uint16_t>(static_cast<uint8_t>(section_data[page_start + page_header_in_order_len_low]))
                  | static_cast<uint16_t>(static_cast<uint8_t>(section_data[page_start + page_header_in_order_len_high]) << byte_shift);
            }
            const auto page_idx = static_cast<uint32_t>(page_start / page_size);

            // Find EOF by advancing instruction-by-instruction so that 0xFF bytes
            // embedded inside instruction arguments (e.g. apply_offset_57 with
            // offset=0xFFFF for an unresolved relocation) are not mistaken for EOF.
            size_t code_end = page_end;
            for (size_t i = code_start; i < page_end;) {
                auto op = static_cast<uint8_t>(section_data[i]);
                if (op == eof_opcode) {
                    code_end = i + eof_size;
                    break;
                }
                if (op == align_opcode) { ++i; continue; }
                auto it = isa_op_map->find(op);
                if (it == isa_op_map->end()) break;  // unknown — stop scan
                // Instruction size: 1 byte opcode + 1 byte implicit pad + sum of arg widths in bytes
                size_t insn_size = 2;
                for (const auto& arg : it->second.get_args())
                    insn_size += static_cast<size_t>(arg.get_width()) / 8;
                if (insn_size < 2) break;
                i += insn_size;
            }

            const bool is_save_page    = state->is_preempt_save_page_id(page_idx);
            const bool is_restore_page = state->is_preempt_restore_page_id(page_idx);

            if (is_restore_page) {
                // Restore pages are entirely auto-generated — skip text and data.
                state->reset();
                continue;
            }

            if (is_save_page) {
                // Save pages are auto-generated from hintmap — skip ctrltext emission.
                // Process ctrltext silently to populate labels (needed for BD decoding).
                // Then decode ctrldata BDs silently to collect remote addresses for hintmap.
                const uint32_t save_slot = state->get_save_page_slot(page_idx);
                state->set_current_save_page(save_slot);
                {
                    std::ostringstream null_sink;
                    asm_writer null_writer(null_sink);
                    // Silently process ctrltext to populate label map
                    isa_op_disasm dummy_isa_op("dummy", 0, std::vector<opArg>{});
                    for (size_t off = code_start; off < code_end;) {
                        auto op = static_cast<uint8_t>(section_data[off]);
                        if (op == align_opcode) { state->increment_address(1); ++off; continue; }
                        auto it = isa_op_map->find(op);
                        if (it == isa_op_map->end()) break;
                        auto deserializer = it->second.create_deserializer();
                        size_t consumed = deserializer->deserialize(null_writer, state, section_data + off);
                        off += consumed;
                    }
                    // Silently process ctrldata to collect BD addresses
                    const size_t data_start = code_end;
                    const size_t data_size  = page_end - data_start;
                    if (data_size > 0)
                        process_data_block_with_writer(section_data + data_start, data_size, state, null_writer);
                }
                state->clear_current_save_page();
                // Hintmap data is emitted in the ctrldata of the last code page before
                // the NOP page (see below) so it stays in the "default" scope.
                state->reset();
                continue;
            }

            // --- text portion ---
            // (state carries labels from opcodes into the data block; do NOT reset between them)
            add_text_sec_comment();

            // For merged-page sections: emit load_pdi target labels AFTER ".section .ctrltext"
            // so the assembler sees them with data_state=false and records them as page-level
            // labels in m_labelpageindex. Emitting before .section .ctrltext would put them in
            // the data section (data_state=true) where they are invisible to load_pdi resolution.
            // Emit load_pdi target label (opens a new scope) if this page is a NOP target.
            {
                std::string load_label = state->consume_load_pdi_label(page_idx);
                if (!load_label.empty()) {
                    m_asm_writer.write_label(load_label);
                    open_label = (load_label[0] == '@') ? load_label.substr(1) : load_label;
                }
            }

            process_text_block(section_data, code_start, code_end, state);

            // --- data portion (after EOF, rest of the page) ---
            // Always emit the ctrldata header so the assembler sees a data section
            // even for pages whose data region is entirely zero-padding.
            const size_t data_start = code_end;
            const size_t data_size  = page_end - data_start;
            add_data_sec_comment();
            if (data_size > 0)
                process_data_block(section_data + data_start, data_size, state);

            // Close the label scope with .endl only when this is the last page in the
            // scope (in_order_page_len == 0).  If non-zero, the next page continues in
            // the same scope so we keep open_label and process it without a new label.
            if (!open_label.empty() && page_in_order_len == 0) {
                m_asm_writer.write_endl(open_label);
                open_label.clear();
            }

            // Emit hintmap data for all save pages in this column in the ctrldata of the
            // last code page before the NOP page.  Hintmaps must be in the "default" scope
            // (not inside a page-scope like "default:label837") so the assembler can find
            // them when resolving @hintmapN references in preempt instructions.
            // The NOP page is identified by having a load_pdi label registered for it.
            if (state->is_load_pdi_target(page_idx + 1)) {
                // Flush hintmaps for save pages belonging to this column only.
                const uint32_t col_start = state->get_column_start_slot();
                const uint32_t col_end   = state->get_current_slot_count();
                for (uint32_t slot = col_start; slot < col_end; ++slot) {
                    emit_save_page_hintmap(slot, *state);
                }
            }

            // Reset state between pages (not between text and data of the same page).
            state->reset();
        }
    } else {
        // Single-page (separate-page) section: the section contains exactly one
        // page's worth of opcodes starting after the 16-byte section header.
        process_text_block(section_data, elf_section_header_padding, section_size, state);
    }
}

void elf_asm_disassembler::process_data_section(const ELFIO::section* section, std::shared_ptr<disassembler_state> state) {
    const char* section_data = section->get_data();
    size_t section_size = section->get_size();
    // Use common base class method for data processing
    process_data_block(section_data, section_size, state);
}

void elf_asm_disassembler::process_pad_section(const ELFIO::section* /*section*/, std::shared_ptr<disassembler_state> /*state*/) {
    std::cout << "Dumping .pad not supported\n";
}

void elf_asm_disassembler::emit_save_page_hintmap(uint32_t page_idx,
                                                   const disassembler_state& state) {
    const std::string hm_label = state.get_hintmap_label(page_idx);
    if (hm_label.empty())
        return;

    hintmap_chunk_bits bs;
    const auto [start_chunk, num_chunks] = state.get_hintmap_chunk_range(page_idx);
    if (num_chunks > 0) {
        for (uint64_t c = start_chunk; c < start_chunk + num_chunks && c < HINTMAP_CHUNK_BITS; ++c)
            bs.set(static_cast<std::size_t>(c));
    }

    // Serialise bitset as .long words, trimming trailing zeros.
    std::vector<uint32_t> words(HINTMAP_WORD_COUNT, 0);
    for (std::size_t bit = 0; bit < HINTMAP_CHUNK_BITS; ++bit) {
        if (bs.test(bit))
            words[bit / HINTMAP_WORD_BITS] |= (1U << (bit % HINTMAP_WORD_BITS));
    }
    // Find last non-zero word to trim trailing zeros.
    std::size_t last_nonzero = HINTMAP_WORD_COUNT;
    while (last_nonzero > 0 && words[last_nonzero - 1] == 0)
        --last_nonzero;

    // Only emit the hintmap label+data when the prescan confirmed valid data for it.
    if (!state.is_hintmap_label_valid(hm_label))
        return;
    const std::size_t emit_count = (last_nonzero > 0) ? last_nonzero : 1;
    m_asm_writer.write_label(hm_label);
    for (std::size_t i = 0; i < emit_count; ++i) {
        std::ostringstream oss;
        oss << ".long 0x" << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << words[i];
        m_asm_writer.write_directive("\t" + oss.str());
    }
}


void elf_asm_disassembler::prescan_hintmap_slots(std::shared_ptr<disassembler_state> state) {
    // Silently replay the merged-page save-page processing for every ctrltext section
    // to populate save_page_bd_addresses and then mark valid_hintmap_slots.
    const unsigned char abi_version = m_elf_reader.get_abi_version();
    if (abi_version < elf_version_config)
        return;  // not merged-page format — no pre-scan needed

    std::ostringstream null_sink;
    asm_writer null_writer(null_sink);
    int current_column = -1;

    for (const auto& section_ptr : m_elf_reader.sections) {
        const ELFIO::section* section = section_ptr.get();
        const std::string section_name = section->get_name();
        if (section->get_type() != ELFIO::SHT_PROGBITS)
            continue;
        if (!is_text_section(section_name))
            continue;

        // Detect column change
        {
            size_t first_dot = section_name.find('.', 1);
            if (first_dot != std::string::npos) {
                size_t second_dot = section_name.find('.', first_dot + 1);
                if (second_dot != std::string::npos) {
                    try {
                        int col = std::stoi(section_name.substr(first_dot + 1, second_dot - first_dot - 1));
                        if (col != current_column) {
                            current_column = col;
                            state->reset_section_page_mappings();
                        }
                    } catch (...) {}
                }
            }
        }

        const char* section_data = section->get_data();
        const size_t section_size = section->get_size();

        for (size_t page_start = 0; page_start < section_size; page_start += page_size) {
            const size_t code_start = page_start + elf_section_header_padding;
            const size_t page_end   = page_start + page_size;
            const auto page_idx = static_cast<uint32_t>(page_start / page_size);

            // Find EOF
            size_t code_end = page_end;
            for (size_t i = code_start; i < page_end;) {
                auto op = static_cast<uint8_t>(section_data[i]);
                if (op == eof_opcode) { code_end = i + eof_size; break; }
                if (op == align_opcode) { ++i; continue; }
                auto it = isa_op_map->find(op);
                if (it == isa_op_map->end()) break;
                size_t insn_size = 2;
                for (const auto& arg : it->second.get_args())
                    insn_size += static_cast<size_t>(arg.get_width()) / 8;
                if (insn_size < 2) break;
                i += insn_size;
            }

            if (state->is_preempt_restore_page_id(page_idx)) {
                state->reset();
                continue;
            }

            if (!state->is_preempt_save_page_id(page_idx)) {
                // Normal page: silently deserialize to register save/restore page IDs
                for (size_t off = code_start; off < code_end;) {
                    auto op = static_cast<uint8_t>(section_data[off]);
                    if (op == align_opcode) { state->increment_address(1); ++off; continue; }
                    auto it = isa_op_map->find(op);
                    if (it == isa_op_map->end()) break;
                    auto deserializer = it->second.create_deserializer();
                    size_t consumed = deserializer->deserialize(null_writer, state, section_data + off);
                    off += consumed;
                }
                state->reset();
                continue;
            }

            // Save page: decode hintmap chunk range from two fixed-offset BDs in ctrldata.
            //
            // Each save page has two DMA BDs at known page-internal offsets whose fields
            // encode the chunk range WITHOUT needing the optional .dump.N section:
            //
            //   BD_A at page offset 0x0eb0:
            //     rem_hi (u32 at BD+12) = (start_chunk + last_chunk + 1) × (CHUNK_SIZE/2)
            //     → total = rem_hi / 0x8000 = 2×start + num_chunks
            //
            //   BD_B at page offset 0x0f10:
            //     lptr   (u32 at BD+4)  = num_chunks × 0x2000
            //     → num_chunks = lptr / 0x2000
            //
            //   start_chunk = (total - num_chunks) / 2
            //
            const uint32_t save_slot = state->get_save_page_slot(page_idx);
            state->mark_hintmap_valid(save_slot);

            // BD_A offset within page: 0x0eb0; rem_hi at +12
            constexpr size_t bd_a_off   = 0x0eb0;
            constexpr size_t bd_b_off   = 0x0f10;
            constexpr uint32_t half_chunk = CHUNK_SIZE / 2;   // 0x8000
            constexpr uint32_t lptr_unit  = 0x2000;

            if (page_start + bd_b_off + 8 <= section_size) {
                const char* d = section_data + page_start;
                uint32_t rem_hi = static_cast<uint8_t>(d[bd_a_off+12])
                                | (static_cast<uint8_t>(d[bd_a_off+13]) << 8)
                                | (static_cast<uint8_t>(d[bd_a_off+14]) << 16)
                                | (static_cast<uint8_t>(d[bd_a_off+15]) << 24);
                uint32_t lptr   = static_cast<uint8_t>(d[bd_b_off+4])
                                | (static_cast<uint8_t>(d[bd_b_off+5]) << 8)
                                | (static_cast<uint8_t>(d[bd_b_off+6]) << 16)
                                | (static_cast<uint8_t>(d[bd_b_off+7]) << 24);
                if (half_chunk > 0 && lptr_unit > 0) {
                    uint64_t total      = rem_hi / half_chunk;
                    uint64_t num_chunks = lptr   / lptr_unit;
                    uint64_t start      = (num_chunks > 0 && total >= num_chunks)
                                        ? (total - num_chunks) / 2 : 0;
                    if (num_chunks > 0)
                        state->set_hintmap_chunk_range(save_slot, start, num_chunks);
                }
            }
            state->reset();
        }
    }
}

bool elf_asm_disassembler::is_text_section(const std::string& section_name) const {
    bool result = section_name.substr(0, ctrltext_string_length) == ".ctrltext";
    return result;
}

bool elf_asm_disassembler::is_data_section(const std::string& section_name) const {
    bool result = section_name.substr(0, ctrldata_string_length) == ".ctrldata";
    return result;
}

void bin_asm_disassembler::process_binary() {
    if (m_binary_data.empty()) {
        throw error(error::error_code::invalid_input, "Binary data is empty\n");
    }

    size_t offset = 0;
    int page_num = 0;

    while (offset < m_binary_data.size()) {
        // Check if there's enough data for a page
        size_t remaining = m_binary_data.size() - offset;
        if (remaining < page_header_size) {
            break; // Not enough data for another page
        }

        // Check for page header magic bytes
        if (static_cast<uint8_t>(m_binary_data[offset + page_header_magic_byte_0]) != page_header_magic ||
            static_cast<uint8_t>(m_binary_data[offset + page_header_magic_byte_1]) != page_header_magic) {
            // No more pages
            break;
        }

        // Read cur_page_len from header
        uint16_t cur_page_len = static_cast<uint8_t>(m_binary_data[offset + page_header_cur_len_low]) |
                               (static_cast<uint8_t>(m_binary_data[offset + page_header_cur_len_high]) << byte_shift);

        // cur_page_len includes the header itself, so content is (cur_page_len
        // - page_header_size)

        const size_t avail = remaining - page_header_size;
        const size_t content_size = (cur_page_len > page_header_size) ?
            (cur_page_len - page_header_size) : 0;
        if (content_size > avail)
            throw error(error::error_code::invalid_asm,
                        "page_len exceeds remaining bytes");

        // Process this page
        auto state = create_disassembler_state();

        if (content_size > 0) {
            process_binary_data(m_binary_data.data() + offset + page_header_size,
                               content_size, state);
        }

        // Move to next page (always at page_size boundary)
        offset += page_size;
        page_num++;
    }

    if (page_num == 0) {
        throw error(error::error_code::invalid_input,
                   "No valid pages found in binary file\n");
    }
}


void bin_asm_disassembler::process_binary_data(const char* data, size_t size, std::shared_ptr<disassembler_state> state) {
    // Process PAGE structure: TEXT section → EOF → DATA section → padding
    // This mirrors how ELF sections are processed

    size_t text_end_offset = 0;
    bool found_eof = false;

    // Find EOF to determine TEXT section boundary
    for (size_t i = 0; i < size; i++) {
        if (static_cast<uint8_t>(data[i]) == eof_opcode) {
            // Check if this is actually an EOF opcode (not just 0xFF in data)
            auto op_it = isa_op_map->find(eof_opcode);
            if (op_it != isa_op_map->end()) {
                text_end_offset = i + eof_size;
                found_eof = true;
                break;
            }
        }
    }

    // If no EOF found, entire file is TEXT section
    if (!found_eof) {
        text_end_offset = size;
    }

    // Process TEXT section (mirroring process_text_section for ELF)
    add_text_sec_comment();

    // Use common base class method for text processing
    process_text_block(data, 0, text_end_offset, state);

    // Process DATA section if present (mirroring process_data_section for ELF)
    if (found_eof && text_end_offset < size) {
        add_data_sec_comment();
        m_asm_writer.write_directive("  .align             " + std::to_string(align_16));

        process_data_section_binary(data + text_end_offset, size - text_end_offset, state);

        // Reset state after DATA section (like ELF disassembler does)
        state->reset();
    }
}

void bin_asm_disassembler::process_data_section_binary(const char* data, size_t size, std::shared_ptr<disassembler_state> state) {
    // Use common base class method for data processing
    process_data_block(data, size, state);
}
} // namespace aiebu
