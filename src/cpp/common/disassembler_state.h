// SPDX-License-Identifier: MIT
// Copyright (C) 2025-2026, Advanced Micro Devices, Inc. All rights reserved.

#ifndef AIEBU_SRC_CPP_COMMON_DISASSEMBLER_STATE_H
#define AIEBU_SRC_CPP_COMMON_DISASSEMBLER_STATE_H

#include <climits>
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <sstream>

#include "aiebu/aiebu_error.h"

namespace aiebu {

// Abstract base class for disassembler state
// Derived classes implement architecture-specific actor ID mappings
class disassembler_state {
private:
    uint32_t position = 0;
    std::map<uint32_t, std::string> labels;
    std::map<uint32_t, std::string> external_labels;
    std::map<uint32_t, std::pair<std::string, uint32_t>> local_ptr;
    std::vector<std::string> pending_ooo_labels;  // Labels from OOO instructions (load_pdi, preempt, load_cores)
    std::set<std::string> preempt_save_labels;    // Labels of preempt save pages (skip ctrltext, decode BD)
    std::set<std::string> preempt_restore_labels; // Labels of preempt restore pages (skip entirely)
    std::set<uint32_t> preempt_restore_page_ids;  // Page index values for preempt restore pages
    // Maps raw page_idx -> unique slot index (monotonically increasing per save page encountered)
    std::map<uint32_t, uint32_t> preempt_save_page_slot;
    uint32_t preempt_save_slot_counter = 0;
    // BD remote addresses collected from save pages, indexed by slot
    std::map<uint32_t, std::vector<uint32_t>> save_page_bd_addresses;
    // Current save page slot being collected (UINT32_MAX = not collecting)
    uint32_t current_save_page_slot = UINT32_MAX;
    // Hintmap label assigned to each slot ("hintmapN")
    std::map<uint32_t, std::string> save_page_hintmap_labels;
    uint32_t hintmap_label_counter = 0;
    // Set of slots whose hintmap bitset is non-zero (valid to emit as 4th arg)
    std::set<uint32_t> valid_hintmap_slots;
    // Set of hintmap label names confirmed valid (populated from prescan results)
    std::set<std::string> valid_hintmap_label_names;
    // Per-slot hintmap chunk range from dump JSON: slot -> (start_chunk, num_chunks)
    std::map<uint32_t, std::pair<uint64_t, uint64_t>> hintmap_chunk_ranges;
    // page_idx -> load_pdi label: for merged-page format, emit label at exact page_idx
    std::map<uint32_t, std::string> page_idx_to_load_label;
    // First slot index belonging to the current column (set on column change)
    uint32_t column_start_slot = 0;

public:
    virtual ~disassembler_state() = default;

    uint32_t get_address() const { return position; }

    void increment_address(uint32_t offset) {
        position += offset;
    }

    const std::map<uint32_t, std::string>& get_labels() const {
        return labels;
    }

    void add_label(uint32_t addr, const std::string& label) {
        labels[addr] = label;
    }

    const std::map<uint32_t, std::pair<std::string, uint32_t>>& get_local_ptrs() const {
        return local_ptr;
    }

    const std::map<uint32_t, std::string>& get_external_labels() const {
        return external_labels;
    }

    void add_external_label(uint32_t address, std::string label) {
        external_labels[address] = std::move(label);
    }

    // OOO (Out-Of-Order) labels from load_pdi, preempt, load_cores instructions
    // These reference text sections that come after the current one
    void add_ooo_label(const std::string& label) {
        pending_ooo_labels.push_back(label);
    }

    bool has_pending_ooo_labels() const {
        return !pending_ooo_labels.empty();
    }

    std::string get_next_ooo_label() {
        if (pending_ooo_labels.empty()) return "";
        std::string label = pending_ooo_labels.front();
        pending_ooo_labels.erase(pending_ooo_labels.begin());
        return label;
    }

    // Register load_pdi OOO label for exact page_idx emission in merged-page format.
    void add_load_pdi_label(uint32_t page_idx, const std::string& label) {
        page_idx_to_load_label[page_idx] = label;
    }

    // Return and clear the load_pdi label for page_idx (empty string if none).
    std::string consume_load_pdi_label(uint32_t page_idx) {
        auto it = page_idx_to_load_label.find(page_idx);
        if (it == page_idx_to_load_label.end()) return "";
        std::string label = it->second;
        page_idx_to_load_label.erase(it);
        return label;
    }

    // Return true if page_idx is a load_pdi target (NOP page).
    bool is_load_pdi_target(uint32_t page_idx) const {
        return page_idx_to_load_label.count(page_idx) > 0;
    }

    // Preempt save/restore page tracking by label and by page index
    void mark_preempt_save_label(const std::string& label) {
        preempt_save_labels.insert(label);
    }

    void mark_preempt_restore_label(const std::string& label) {
        preempt_restore_labels.insert(label);
    }

    // Register page_id as a save page in the current section.
    // A new globally-unique slot (and hintmap label) is allocated only once per
    // page_id per section; repeated calls with the same page_id are no-ops.
    // All preempts in a column share a single @save / @restore label (matching
    // the reference ASM convention).  page_id is ignored for label naming.
    std::string alloc_save_label(uint32_t page_id) {
        (void)page_id;
        return "@save";
    }
    std::string alloc_restore_label(uint32_t page_id) {
        (void)page_id;
        return "@restore";
    }

    void mark_preempt_save_page_id(uint32_t page_id) {
        if (preempt_save_page_slot.count(page_id))
            return;  // already registered in this section
        uint32_t slot = preempt_save_slot_counter++;
        preempt_save_page_slot[page_id] = slot;
        save_page_hintmap_labels[slot] = "hintmap_" + std::to_string(hintmap_label_counter++);
    }

    // Return the slot assigned to page_id (the most recently registered one).
    uint32_t get_save_page_slot(uint32_t page_id) const {
        auto it = preempt_save_page_slot.find(page_id);
        return (it != preempt_save_page_slot.end()) ? it->second : UINT32_MAX;
    }

    // Return the hintmap label for slot (used by disassembler), or "" if unknown.
    std::string get_hintmap_label(uint32_t slot) const {
        auto it = save_page_hintmap_labels.find(slot);
        return (it != save_page_hintmap_labels.end()) ? it->second : "";
    }

    // Mark slot as having a non-zero hintmap (called after prescan confirms valid data).
    void mark_hintmap_valid(uint32_t slot) {
        valid_hintmap_slots.insert(slot);
        // Also record the label name so the main pass can look it up by name.
        auto it = save_page_hintmap_labels.find(slot);
        if (it != save_page_hintmap_labels.end())
            valid_hintmap_label_names.insert(it->second);
    }

    // True if the slot has been confirmed to have non-zero hintmap data.
    bool is_hintmap_valid(uint32_t slot) const { return valid_hintmap_slots.count(slot) > 0; }

    // True if the hintmap label name is confirmed valid (for main pass lookup by label).
    bool is_hintmap_label_valid(const std::string& label) const {
        return valid_hintmap_label_names.count(label) > 0;
    }

    // Merge prescan validity results into this state (called after prescan on separate state).
    void import_valid_hintmap_labels(const std::set<std::string>& names) {
        valid_hintmap_label_names.insert(names.begin(), names.end());
    }

    const std::set<std::string>& get_valid_hintmap_label_names() const {
        return valid_hintmap_label_names;
    }

    // Store the chunk range for a slot (decoded from save page BD fields).
    void set_hintmap_chunk_range(uint32_t slot, uint64_t start_chunk, uint64_t num_chunks) {
        hintmap_chunk_ranges[slot] = {start_chunk, num_chunks};
        // Mark valid since we have real data
        valid_hintmap_slots.insert(slot);
        const auto it = save_page_hintmap_labels.find(slot);
        if (it != save_page_hintmap_labels.end())
            valid_hintmap_label_names.insert(it->second);
    }

    // Return the chunk range for a slot, or {0,0} if not known.
    std::pair<uint64_t, uint64_t> get_hintmap_chunk_range(uint32_t slot) const {
        auto it = hintmap_chunk_ranges.find(slot);
        return (it != hintmap_chunk_ranges.end()) ? it->second : std::make_pair<uint64_t, uint64_t>(0, 0);
    }

    // Merge chunk ranges from another state (used to copy prescan results).
    void import_hintmap_chunk_ranges(const std::map<uint32_t, std::pair<uint64_t,uint64_t>>& ranges) {
        hintmap_chunk_ranges.insert(ranges.begin(), ranges.end());
    }

    const std::map<uint32_t, std::pair<uint64_t,uint64_t>>& get_hintmap_chunk_ranges() const {
        return hintmap_chunk_ranges;
    }

    void mark_preempt_restore_page_id(uint32_t page_id) {
        preempt_restore_page_ids.insert(page_id);
    }

    bool is_preempt_save_page(const std::string& label) const {
        return preempt_save_labels.count(label) > 0;
    }

    bool is_preempt_restore_page(const std::string& label) const {
        return preempt_restore_labels.count(label) > 0;
    }

    bool is_preempt_save_page_id(uint32_t page_id) const {
        return preempt_save_page_slot.count(page_id) > 0;
    }

    bool is_preempt_restore_page_id(uint32_t page_id) const {
        return preempt_restore_page_ids.count(page_id) > 0;
    }

    // Track BD remote addresses from the save page to reconstruct hintmap.
    // set_current_save_page/clear_current_save_page use the slot (not page_id).
    void set_current_save_page(uint32_t slot) { current_save_page_slot = slot; }
    void clear_current_save_page() { current_save_page_slot = UINT32_MAX; }
    bool is_collecting_save_bd() const { return current_save_page_slot != UINT32_MAX; }

    void add_save_page_bd_address(uint32_t remote_addr_low) {
        if (current_save_page_slot != UINT32_MAX)
            save_page_bd_addresses[current_save_page_slot].push_back(remote_addr_low);
    }

    const std::vector<uint32_t>& get_save_page_bd_addresses(uint32_t slot) const {
        static const std::vector<uint32_t> empty;
        auto it = save_page_bd_addresses.find(slot);
        return (it != save_page_bd_addresses.end()) ? it->second : empty;
    }

    const std::map<uint32_t, std::vector<uint32_t>>& get_all_save_bd_addresses() const { return save_page_bd_addresses; }

    void add_local_ptr(uint32_t address, const std::string& label, uint32_t offset) {
        local_ptr[address] = std::make_pair(label, offset);
    }

    // Clear per-section page mappings (restore/save page_id → slot) so that the
    // same page_idx values in a new ctrltext section don't collide with prior ones.
    // Counters and label assignments persist across sections.
    void reset_section_page_mappings() {
        preempt_save_page_slot.clear();
        preempt_restore_page_ids.clear();
        page_idx_to_load_label.clear();
        // Note: pending_ooo_labels is NOT cleared here — it's consumed by the outer
        // process_sections() loop for the separate-page (legacy) ELF format.
        // Record where this column's slots start so hintmap emission stays per-column.
        column_start_slot = preempt_save_slot_counter;
    }

    uint32_t get_column_start_slot() const { return column_start_slot; }
    uint32_t get_current_slot_count() const { return preempt_save_slot_counter; }

    void reset() {
        position = 0;
        labels.clear();
        external_labels.clear();
        local_ptr.clear();
    }

    std::string to_string() const {
        std::ostringstream oss;
        oss << "[POS:" << position
            << "\tLABELS:" << labels.size()
            << "\tLOCAL_PTRS:" << local_ptr.size() << "]";
        return oss.str();
    }

    // Common tile conversion - same for all architectures
    std::string to_tile(uint32_t arg) const {
      uint32_t row = arg & 0x1F;   //NOLINT
      uint32_t col = (arg >> 5) & 0x7F;  //NOLINT
      return "TILE_" + std::to_string(col) + "_" + std::to_string(row);
    }

    // Pure virtual - architecture-specific actor ID mapping
    virtual std::string to_actor(uint32_t val, uint32_t tile) const = 0;
};

// AIE2PS-specific disassembler state
class disassembler_state_aie2ps : public disassembler_state {
public:
    std::string to_actor(uint32_t val, uint32_t tile) const override {
        uint32_t row = tile & 0x1F;  //NOLINT

        if (row == 0) {  //NOLINT
          // One SHIM TILE
          switch(val)
          {
            case 0: return "SHIM_S2MM_0"; //NOLINT
            case 1: return "SHIM_S2MM_1"; //NOLINT
            case 6: return "SHIM_MM2S_0"; //NOLINT
            case 7: return "SHIM_MM2S_1"; //NOLINT
            default: throw error(error::error_code::invalid_asm, "Invalid AIE2PS Shim tile actor:" + std::to_string(val) + "\n");
          }
        }
        else if (row == 1 || row == 2) { //NOLINT
          // Two MEM TILE
          switch(val)
          {
            case 0: return "MEM_S2MM_0"; //NOLINT
            case 1: return "MEM_S2MM_1"; //NOLINT
            case 2: return "MEM_S2MM_2"; //NOLINT
            case 3: return "MEM_S2MM_3"; //NOLINT
            case 4: return "MEM_S2MM_4"; //NOLINT
            case 5: return "MEM_S2MM_5"; //NOLINT
            case 6: return "MEM_MM2S_0"; //NOLINT
            case 7: return "MEM_MM2S_1"; //NOLINT
            case 8: return "MEM_MM2S_2"; //NOLINT
            case 9: return "MEM_MM2S_3"; //NOLINT
            case 10: return "MEM_MM2S_4"; //NOLINT
            case 11: return "MEM_MM2S_5"; //NOLINT
            default: throw error(error::error_code::invalid_asm, "Invalid AIE2PS Mem tile actor:" + std::to_string(val) + "\n");
          }
        }
        else { // CORE TILE
          switch(val)
          {
            case 0: return "TILE_S2MM_0"; //NOLINT
            case 1: return "TILE_S2MM_1"; //NOLINT
            case 6: return "TILE_MM2S_0"; //NOLINT
            case 7: return "TILE_MM2S_1"; //NOLINT
            case 15: return "TILE_CORE"; //NOLINT
            default: throw error(error::error_code::invalid_asm, "Invalid AIE2PS Core tile actor:" + std::to_string(val) + "\n");
          }
        }
    }
};

// AIE4-specific disassembler state with extended actor ID mappings
class disassembler_state_aie4 : public disassembler_state {
public:
    std::string to_actor(uint32_t val, uint32_t tile) const override {
        uint32_t row = tile & 0x1F;  //NOLINT

        if (row == 0) {  //NOLINT
          // SHIM TILE - AIE4 has more channels + control channels
          switch(val)
          {
            case 0: return "SHIM_S2MM_0"; //NOLINT
            case 1: return "SHIM_TRACE_S2MM"; //NOLINT
            case 2: return "SHIM_S2MM_1"; //NOLINT
            case 6: return "SHIM_MM2S_0"; //NOLINT
            case 7: return "SHIM_MM2S_1"; //NOLINT
            case 8: return "SHIM_MM2S_2"; //NOLINT
            case 9: return "SHIM_MM2S_3"; //NOLINT
            case 16: return "SHIM_CTRL_MM2S_0"; //NOLINT
            case 17: return "SHIM_CTRL_MM2S_1"; //NOLINT
            default: throw error(error::error_code::invalid_asm, "Invalid AIE4 Shim tile actor:" + std::to_string(val) + "\n");
          }
        }
        else if (row == 1 || row == 2) { //NOLINT
          // MEM TILE - AIE4 has more channels (0-7 for S2MM, 16-26 for MM2S)
          switch(val)
          {
            case 0: return "MEM_S2MM_0"; //NOLINT
            case 1: return "MEM_S2MM_1"; //NOLINT
            case 2: return "MEM_S2MM_2"; //NOLINT
            case 3: return "MEM_S2MM_3"; //NOLINT
            case 4: return "MEM_S2MM_4"; //NOLINT
            case 5: return "MEM_S2MM_5"; //NOLINT
            case 6: return "MEM_S2MM_6"; //NOLINT
            case 7: return "MEM_S2MM_7"; //NOLINT
            case 16: return "MEM_MM2S_0"; //NOLINT
            case 17: return "MEM_MM2S_1"; //NOLINT
            case 18: return "MEM_MM2S_2"; //NOLINT
            case 19: return "MEM_MM2S_3"; //NOLINT
            case 20: return "MEM_MM2S_4"; //NOLINT
            case 22: return "MEM_MM2S_5"; //NOLINT
            case 23: return "MEM_MM2S_6"; //NOLINT
            case 24: return "MEM_MM2S_7"; //NOLINT
            case 25: return "MEM_MM2S_8"; //NOLINT
            case 26: return "MEM_MM2S_9"; //NOLINT
            default: throw error(error::error_code::invalid_asm, "Invalid AIE4 Mem tile actor:" + std::to_string(val) + "\n");
          }
        }
        else { // CORE TILE - Same as AIE2PS
          switch(val)
          {
            case 0: return "TILE_S2MM_0"; //NOLINT
            case 1: return "TILE_S2MM_1"; //NOLINT
            case 6: return "TILE_MM2S_0"; //NOLINT
            case 15: return "TILE_CORE"; //NOLINT
            default: throw error(error::error_code::invalid_asm, "Invalid AIE4 Core tile actor:" + std::to_string(val) + "\n");
          }
        }
    }
};

}// namespace aiebu

#endif //AIEBU_COMMON_DISASSEMBLER_STATE_H_
