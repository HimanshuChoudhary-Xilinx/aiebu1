# AIEBU User Manual

**AIEBU** — AI Engine Binary Utilities — is a library and set of command-line tools for assembling, packaging, inspecting, and disassembling AI Engine (AIE) control code into ELF files used by XRT at runtime.

---

## Table of Contents

1. [Overview](#overview)
2. [Concepts](#concepts)
3. [C API — aiebu.h](#c-api)
4. [C++ API — aiebu_assembler.h](#cpp-api)
   - [buffer_type enum](#buffer_type-enum)
   - [Constructors](#constructors)
   - [get_elf()](#get_elf)
   - [get_report() and disassemble()](#get_report-and-disassemble)
   - [get_argtbl() and flush_argtbl()](#get_argtbl-and-flush_argtbl)
   - [get_op_locations()](#get_op_locations)
   - [file_artifact](#file_artifact)
5. [Error Handling](#error-handling)
6. [aiebu-asm — Assembler Tool](#aiebu-asm)
   - [Global options](#asm-global-options)
   - [Target: aie2txn](#target-aie2txn)
   - [Target: aie2dpu](#target-aie2dpu)
   - [Target: aie2asm](#target-aie2asm)
   - [Target: aie2ps](#target-aie2ps)
   - [Target: aie4](#target-aie4)
   - [Target: aie2_config](#target-aie2_config)
   - [Target: aie2ps_config](#target-aie2ps_config)
   - [Target: aie4_config](#target-aie4_config)
7. [aiebu-dump — Inspection Tool](#aiebu-dump)
   - [Options](#dump-options)
   - [Usage examples](#dump-usage-examples)
8. [ELF Output Sections](#elf-output-sections)
9. [Common Workflows](#common-workflows)

---

## Overview

AIEBU has two roles:

- **Assembler** — Takes AIE control code (binary blobs, ASM text, or a config JSON) and produces an ELF file that XRT can load and patch at runtime.
- **Inspector** — Takes an ELF or binary file and prints headers, disassembly, symbol tables, relocation entries, and debug information.

The library exposes both a C API (`aiebu.h`) and a C++ API (`aiebu_assembler.h`). The command-line tools `aiebu-asm` and `aiebu-dump` are thin wrappers around the same library.

---

## Concepts

**buffer_type** — Identifies what kind of input is being assembled and what kind of ELF to emit. Choosing the right buffer type is the first decision in any workflow.

**patch_json / external_buffer_id.json** — A JSON file that tells the assembler which symbols in the ELF should be patched by XRT at runtime, and how.

**control packet** — A binary blob placed in the `.ctrldata` ELF section. Used alongside transaction control code.

**PM control packet** — A program memory control packet. Multiple PM packets can be embedded in a single ELF, each identified by an integer ID.

**lib / libpath** — Precompiled ELF objects can be linked in at assemble time. `libpath` lists directories to search.

**config JSON** — For full-config ELFs (aie2ps_config, aie4_config), a JSON file describes all kernels, their instances, and the ASM files that implement them.

**file_artifact** — An in-memory virtual filesystem. Pass files as buffers instead of on-disk paths. Used for fully in-memory assembly workflows.

---

## C API

Header: `aiebu/aiebu.h`

### aiebu_assembler_get_elf

```c
int aiebu_assembler_get_elf(
    enum aiebu_assembler_buffer_type type,
    const char*       buffer1,
    size_t            buffer1_size,
    const char*       buffer2,
    size_t            buffer2_size,
    void**            elf_buf,
    const char*       patch_json,
    size_t            patch_json_size,
    const char*       libs,
    const char*       libpaths,
    struct pm_ctrlpkt* pm_ctrlpkts,
    size_t            pm_ctrlpkt_size
);
```

Returns the ELF size on success, or a negative POSIX error code on failure. The ELF is written into a newly allocated buffer pointed to by `*elf_buf`. The caller owns that memory.

**Parameters:**

| Parameter | Description |
|-----------|-------------|
| `type` | Buffer type (see enum below) |
| `buffer1` | Primary input buffer (instruction/transaction code or ASM text) |
| `buffer1_size` | Size of `buffer1` in bytes |
| `buffer2` | Secondary buffer (control packet binary), or `NULL` |
| `buffer2_size` | Size of `buffer2`, or `0` if unused |
| `elf_buf` | Output: pointer to allocated ELF buffer |
| `patch_json` | Contents of the `external_buffer_id.json` patch file, or `NULL` |
| `patch_json_size` | Size of `patch_json`, or `0` if unused |
| `libs` | Semicolon-separated list of library names to link in, or `NULL` |
| `libpaths` | Semicolon-separated list of library search directories, or `NULL` |
| `pm_ctrlpkts` | Array of PM control packets, or `NULL` |
| `pm_ctrlpkt_size` | Number of elements in `pm_ctrlpkts` |

**Buffer type enum (C):**

```c
enum aiebu_assembler_buffer_type {
    aiebu_assembler_buffer_type_blob_instr_dpu,
    aiebu_assembler_buffer_type_blob_instr_prepost,
    aiebu_assembler_buffer_type_blob_instr_transaction,
    aiebu_assembler_buffer_type_blob_control_packet,
    aiebu_assembler_buffer_type_asm_aie2ps,
    aiebu_assembler_buffer_type_asm_aie2,
    aiebu_assembler_buffer_type_asm_aie4,
    aiebu_assembler_buffer_type_aie2_config,
    aiebu_assembler_buffer_type_aie2ps_config,
    aiebu_assembler_buffer_type_aie4_config
};
```

**PM control packet struct (C):**

```c
struct pm_ctrlpkt {
    uint32_t    pm_id;          // Numeric ID for this PM packet
    const char* pm_buffer;      // Binary data
    size_t      pm_buffer_size; // Data length in bytes
};
```

**Example — transaction binary + control packet:**

```c
#include "aiebu/aiebu.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    // Load your binary files into buffers (not shown for brevity)
    const char* txn_data     = /* ... */;
    size_t      txn_size     = /* ... */;
    const char* ctrlpkt_data = /* ... */;
    size_t      ctrlpkt_size = /* ... */;
    const char* patch_json   = /* ... */;
    size_t      patch_size   = /* ... */;

    void* elf_buf = NULL;
    int elf_size = aiebu_assembler_get_elf(
        aiebu_assembler_buffer_type_blob_instr_transaction,
        txn_data,     txn_size,
        ctrlpkt_data, ctrlpkt_size,
        &elf_buf,
        patch_json,   patch_size,
        NULL,  // no libs
        NULL,  // no libpaths
        NULL,  // no PM control packets
        0
    );

    if (elf_size < 0) {
        fprintf(stderr, "aiebu error: %d\n", elf_size);
        return 1;
    }

    FILE* f = fopen("output.elf", "wb");
    fwrite(elf_buf, 1, (size_t)elf_size, f);
    fclose(f);
    free(elf_buf);
    return 0;
}
```

---

## C++ API

Header: `aiebu/aiebu_assembler.h`

All classes and functions live in namespace `aiebu`. Errors are thrown as `aiebu::error` (see [Error Handling](#error-handling)).

### buffer_type enum

```cpp
enum class aiebu_assembler::buffer_type {
    // Binary blob inputs
    blob_instr_dpu,              // DPU control code binary
    blob_instr_prepost,          // Pre/post instruction binary
    blob_instr_transaction,      // Transaction control code binary
    blob_control_packet,         // Control packet binary (AIE2P)
    blob_control_packet_aie2,    // Control packet binary (AIE2)
    blob_aie2ps,                 // Raw binary for AIE2PS
    blob_aie4,                   // Raw binary for AIE4
    blob_aie4a,                  // Raw binary for AIE4A
    blob_aie4z,                  // Raw binary for AIE4Z

    // ASM text inputs
    asm_aie2ps,   // AIE2PS assembly text
    asm_aie2,     // AIE2 assembly text
    asm_aie4,     // AIE4 family assembly text (.target directive selects variant)
    asm_aie4a,    // AIE4A assembly text
    asm_aie4z,    // AIE4Z assembly text

    // Full-config JSON inputs (aiebu builds the entire ELF from a descriptor)
    aie2_config,   // AIE2 config ELF
    aie2ps_config, // AIE2PS config ELF
    aie4_config,   // AIE4 config ELF (variant from .target)
    aie4a_config,  // AIE4A config ELF
    aie4z_config,  // AIE4Z config ELF

    // ELF inputs (for analysis / argtbl transform)
    elf_aie2,      elf_aie2ps,    elf_aie4,    elf_aie4a,    elf_aie4z,
    elf_aie2_config, elf_aie2ps_config, elf_aie4_config,
    elf_aie4a_config, elf_aie4z_config,

    // PDI inputs
    pdi_aie2,   pdi_aie2ps,

    unspecified,
};
```

---

### Constructors

There are five constructor overloads, each suited to a different input scenario.

---

#### Constructor 1 — Two buffers (transaction / DPU / ASM + optional control packet)

```cpp
aiebu_assembler(
    buffer_type                          type,
    const std::vector<char>&             buffer1,
    const std::vector<char>&             buffer2,
    const std::vector<char>&             patch_json,
    const std::vector<std::string>&      libs      = {},
    const std::vector<std::string>&      libpaths  = {},
    const std::map<uint32_t,
                   std::vector<char>>&   pm_ctrlpkt = {}
);
```

Use this when you have a transaction binary (`buffer1`) and optionally a control packet binary (`buffer2`).

| Parameter | Description |
|-----------|-------------|
| `type` | Typically `blob_instr_transaction` or `blob_instr_dpu` |
| `buffer1` | Transaction / instruction binary |
| `buffer2` | Control packet binary, or empty vector |
| `patch_json` | Contents of `external_buffer_id.json`, or empty |
| `libs` | Library names to link |
| `libpaths` | Directories to search for libraries |
| `pm_ctrlpkt` | Map of `{pm_id → binary buffer}` for PM control packets |

**Example — transaction binary only:**

```cpp
#include "aiebu/aiebu_assembler.h"
#include <fstream>
#include <vector>

std::vector<char> read_file(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(f), {}};
}

int main()
{
    auto txn = read_file("ml_txn.bin");

    aiebu::aiebu_assembler as(
        aiebu::aiebu_assembler::buffer_type::blob_instr_transaction,
        txn,                     // buffer1: transaction code
        {},                      // buffer2: no control packet
        {}                       // patch_json: no patching
    );

    auto elf = as.get_elf();

    std::ofstream out("output.elf", std::ios::binary);
    out.write(elf.data(), elf.size());
}
```

**Example — transaction binary + control packet + patch JSON + PM packet:**

```cpp
auto txn      = read_file("ml_txn.bin");
auto ctrlpkt  = read_file("ctrl_pkt0.bin");
auto patch    = read_file("external_buffer_id.json");
auto pm_data  = read_file("pm_ctrl.bin");

std::map<uint32_t, std::vector<char>> pm_map;
pm_map[0] = pm_data;   // PM packet with ID = 0

aiebu::aiebu_assembler as(
    aiebu::aiebu_assembler::buffer_type::blob_instr_transaction,
    txn,
    ctrlpkt,
    patch,
    {},          // libs
    {},          // libpaths
    pm_map
);

auto elf = as.get_elf();
```

---

#### Constructor 2 — Single buffer (ASM text or simple blob)

```cpp
aiebu_assembler(
    buffer_type                      type,
    const std::vector<char>&         buffer,
    const std::vector<std::string>&  libs      = {},
    const std::vector<std::string>&  libpaths  = {},
    const std::vector<char>&         patch_json = {}
);
```

Use this for ASM text assembly (`asm_aie2ps`, `asm_aie2`, `asm_aie4`) where only one primary buffer is needed, or for config ELFs where `buffer` can be empty.

**Example — AIE2PS ASM text assembly:**

```cpp
auto asm_buf = read_file("kernel.asm");

aiebu::aiebu_assembler as(
    aiebu::aiebu_assembler::buffer_type::asm_aie2ps,
    asm_buf,
    {},           // no libs
    {"/opt/aiebu/lib", "build/lib"}  // libpaths (searched for .include and linked libs)
);

auto elf = as.get_elf();
```

**Example — AIE4 ASM text assembly with flags:**

```cpp
auto asm_buf = read_file("compute.asm");

// Note: asm_aie2ps constructor signature has flags before libpaths
aiebu::aiebu_assembler as(
    aiebu::aiebu_assembler::buffer_type::asm_aie2ps,
    asm_buf,
    {"disabledump"},       // flags: suppress .dump debug section
    {"./lib"}              // libpaths
);
```

> For `asm_aie2ps` and `asm_aie4`, the second constructor maps to:
> `aiebu_assembler(type, buffer, libs_or_flags, libpaths, patch_json)`.
> When passing flags, place them in the `libs` position and provide `libpaths` separately.
> See [Constructor 3](#constructor-3--config-json-with-flags) for the explicit-flag variant.

---

#### Constructor 3 — Config JSON with flags

```cpp
aiebu_assembler(
    buffer_type                      type,
    const std::vector<char>&         config_json_buffer,
    const file_artifact&             artifact,
    const std::vector<std::string>&  flags
);
```

Use this for fully in-memory assembly of config ELFs (`aie2ps_config`, `aie4_config`). All referenced files (ASM, control packets) are supplied via `file_artifact` instead of from disk.

| Parameter | Description |
|-----------|-------------|
| `type` | `aie2ps_config`, `aie4_config`, or related config type |
| `config_json_buffer` | Contents of the `config.json` descriptor |
| `artifact` | In-memory virtual filesystem containing all referenced files |
| `flags` | Assembly flags: `disabledump`, `fulldump`, `opt_level_1`–`opt_level_4`, `loglevel_*` |

**Example — AIE4 config ELF fully in-memory:**

```cpp
#include "aiebu/aiebu_assembler.h"

auto config_json = read_file("config.json");
auto main_asm    = read_file("main.asm");
auto ctrlpkt_asm = read_file("ctrlpkt.asm");
auto pdi_asm     = read_file("pdi.asm");
auto ext_buf_id  = read_file("external_buffer_id.json");
auto ctrlpkt_bin = read_file("ctrlpkt0.dat");

aiebu::file_artifact resolver;
resolver.add_vfile("main.asm",               main_asm);
resolver.add_vfile("ctrlpkt.asm",            ctrlpkt_asm);
resolver.add_vfile("pdi.asm",                pdi_asm);
resolver.add_vfile("external_buffer_id.json", ext_buf_id);
resolver.add_vfile("ctrlpkt0.dat",           ctrlpkt_bin);

aiebu::aiebu_assembler as(
    aiebu::aiebu_assembler::buffer_type::aie4_config,
    config_json,
    resolver,
    {}    // no flags (keeps .dump section for debug)
);

auto elf = as.get_elf();
std::ofstream out("output.elf", std::ios::binary);
out.write(elf.data(), elf.size());
```

---

#### Constructor 4 — Load from ELF file path

```cpp
explicit aiebu_assembler(const std::string& elf_path);
```

Loads an existing ELF from disk. Use this to call `get_argtbl()`, `get_op_locations()`, or `get_report()` on an already-assembled ELF.

```cpp
aiebu::aiebu_assembler as("output.elf");
auto tbl = as.get_argtbl("DPU");
```

---

#### Constructor 5 — Load from ELF buffer

```cpp
explicit aiebu_assembler(const std::vector<char>& buffer);
explicit aiebu_assembler(ELFIO::elfio* elf);
explicit aiebu_assembler(const ELFIO::elfio* elf);
```

Same as the file-path constructor but from an in-memory ELF buffer or an already-loaded ELFIO object.

```cpp
auto elf_data = read_file("output.elf");
aiebu::aiebu_assembler as(elf_data);
as.get_report(std::cout);
```

---

### get_elf()

```cpp
std::vector<char> get_elf() const;
```

Returns the assembled ELF as a byte vector. Call after any assembling constructor.

```cpp
auto elf = as.get_elf();
std::ofstream out("result.elf", std::ios::binary);
out.write(elf.data(), elf.size());
```

---

### get_report() and disassemble()

```cpp
void get_report(std::ostream& stream) const;
void disassemble(const std::filesystem::path& root) const;
```

`get_report()` writes a human-readable summary of the assembled ELF (sections, symbols, opcode frequencies) to `stream`.

`disassemble()` writes disassembly files to the directory `root`. The output file names are derived from the ELF section names.

```cpp
aiebu::aiebu_assembler as(
    aiebu::aiebu_assembler::buffer_type::blob_instr_transaction,
    txn, ctrlpkt, patch);

as.get_report(std::cout);

std::filesystem::path dump_dir("./disasm_output");
std::filesystem::create_directories(dump_dir);
as.disassemble(dump_dir);
```

---

### get_argtbl() and flush_argtbl()

These two functions let you read and rewrite the XRT argument-to-buffer-descriptor mapping inside a full config ELF (AIE2PS or AIE4 only).

```cpp
argtbl get_argtbl(const std::string& kernel_name);
void   flush_argtbl(const argtbl& arg_table);
```

**Workflow:**

1. Assemble or load the ELF.
2. Call `get_argtbl("KernelName")` to retrieve the current mapping.
3. Modify `xrt_idx` or `bd_offset` values in place.
4. Optionally rename the kernel with `tbl.set_name("NewName")`.
5. Call `flush_argtbl(tbl)` to write changes back.
6. Call `get_elf()` again to obtain the updated ELF.

**Data structures:**

```
argtbl
 └─ std::vector<instinfo>           (one per kernel instance)
      ├─ inst_name: std::string
      └─ inst_arginfo: std::vector<arginfo>
           ├─ xrt_idx:  uint32_t    (XRT buffer index)
           └─ bd_offset: uint64_t   (BD offset in control code)
```

**Example — rename kernel and remap a buffer argument:**

```cpp
aiebu::aiebu_assembler as(
    aiebu::aiebu_assembler::buffer_type::aie2ps_config,
    {},
    {"disabledump"},
    {testcase_path},
    config_json
);

// Save original ELF
auto elf_orig = as.get_elf();

// Retrieve the argument table for kernel "DPU"
auto tbl = as.get_argtbl("DPU");

// Get a reference to instance 0's argument list
auto& inst  = tbl.get();
auto& table = inst[0].inst_arginfo;

// Rename the kernel
tbl.set_name("CONV");

// Remap xrt_idx for several BD entries
table[1].xrt_idx  = 45;
table[1].bd_offset = 100;
table[5].xrt_idx  = 45;

// Write changes back and get the updated ELF
as.flush_argtbl(tbl);
auto elf_modified = as.get_elf();

std::ofstream out("modified.elf", std::ios::binary);
out.write(elf_modified.data(), elf_modified.size());
```

---

### get_op_locations()

Scans the `.dump` debug section of an ELF and returns source-level locations (file name + line number) for every occurrence of a given opcode.

```cpp
// Config ELF overload — supply kernel name
op_tbl get_op_locations(uint8_t opcode, const std::string& kernel_name) const;

// Target ELF overload — no kernel name
op_tbl get_op_locations(uint8_t opcode) const;
```

Requires the ELF to have been assembled **without** the `disabledump` flag.

**Data structures:**

```
op_tbl
 └─ std::vector<op_loc>
      ├─ inst_name: std::string          (empty for target ELFs)
      └─ line_info: std::vector<lineinfo>
           ├─ col: uint32_t              (AIE column number)
           └─ entries: vector<pair<uint32_t, string>>
                  first  = line number
                  second = source file path
```

**Example — find all SAVE_TIMESTAMPS (opcode 0x1c) in a config ELF:**

```cpp
// Step 1: assemble the ELF (must NOT pass "disabledump")
aiebu::aiebu_assembler as_build(
    aiebu::aiebu_assembler::buffer_type::aie4_config,
    {},
    {},             // flags: empty → .dump section is written
    {test_dir},
    config_json
);

// Step 2: load the ELF back for querying
auto elf_data = as_build.get_elf();
aiebu::aiebu_assembler as(elf_data);

// Step 3: query opcode locations
auto tbl = as.get_op_locations(0x1c, "DPU");

for (const auto& loc : tbl.get_line_info()) {
    std::cout << "Instance: " << loc.inst_name << "\n";
    for (const auto& li : loc.line_info) {
        std::cout << "  Column " << li.col << ":\n";
        for (const auto& [line, file] : li.entries)
            std::cout << "    " << file << ":" << line << "\n";
    }
}
```

**Example — find opcode in a standalone target ELF:**

```cpp
auto asm_buf = read_file("kernel.asm");
aiebu::aiebu_assembler as_build(
    aiebu::aiebu_assembler::buffer_type::asm_aie4,
    asm_buf,
    std::vector<std::string>{},   // no flags
    std::vector<std::string>{test_dir}
);

auto elf_data = as_build.get_elf();
aiebu::aiebu_assembler as(elf_data);

auto tbl = as.get_op_locations(0x1c);  // no kernel name for target ELF
```

---

### file_artifact

`file_artifact` is an in-memory virtual filesystem. Use it to supply referenced files (included ASM, control packets, patch JSON) entirely from memory, without writing anything to disk.

```cpp
class file_artifact {
public:
    // Add a file by name. Buffer is copied; caller retains ownership.
    void add_vfile(const std::string& name, const std::vector<char>& buffer);

    // Add a file by name. Buffer is moved; ownership transfers to artifact.
    void add_vfile(std::string& name, std::vector<char>&& buffer);

    // Retrieve a virtual file by name (throws if not found).
    const std::vector<char>& get(const std::string& name) const;

    // Retrieve a virtual file or fall back to searching paths on disk.
    std::vector<char> get(const std::string& name,
                          const std::vector<std::string>& paths) const;
};
```

The name used in `add_vfile()` must match the filename referenced by the assembler (e.g. the string in a `.include "helper.asm"` directive, or `ctrl_code_file` in `config.json`).

**Rules:**
- Registering the same name twice throws `aiebu::error` with `invalid_input`.
- A `.include` directive for a name already included in the same compilation also throws `invalid_input` (duplicate include protection).

**Example:**

```cpp
aiebu::file_artifact resolver;

// Use move semantics for large buffers to avoid copies
auto pdi_asm = read_file("pdi.asm");
resolver.add_vfile("pdi.asm", std::move(pdi_asm));

// Use copy semantics when the buffer is still needed afterwards
auto helper = read_file("helper.asm");
resolver.add_vfile("helper.asm", helper);
// helper vector is still valid here
```

---

## Error Handling

All errors are thrown as `aiebu::error`, which inherits from `std::system_error`.

```cpp
#include "aiebu/aiebu_error.h"

try {
    aiebu::aiebu_assembler as(type, buffer1, buffer2, patch_json);
    auto elf = as.get_elf();
}
catch (const aiebu::error& ex) {
    std::cerr << "aiebu error: " << ex.what() << "\n";
    std::cerr << "error code:  " << ex.get_code() << "\n";
}
catch (const std::exception& ex) {
    std::cerr << "unexpected error: " << ex.what() << "\n";
}
```

**Error codes:**

| Code | Name | Meaning |
|------|------|---------|
| 1 | `invalid_asm` | Assembly text parsing failure |
| 2 | `invalid_patch_schema` | Malformed patch/external_buffer_id JSON |
| 3 | `invalid_patch_buffer_type` | Incompatible buffer type combination |
| 4 | `invalid_buffer_type` | Unsupported or wrong buffer type |
| 5 | `invalid_offset` | Invalid memory offset in control code |
| 6 | `internal_error` | Internal library error |
| 7 | `invalid_input` | Bad parameter (e.g. duplicate vfile name) |
| 8 | `invalid_elf` | Malformed or unsupported ELF input |
| 9 | `invalid_opcode` | Unknown ISA opcode |

The numeric code is retrieved with `ex.get_code()`.

---

## aiebu-asm

`aiebu-asm` assembles control code into an ELF file. It requires a target (`-t`) that selects which architecture and input format to use.

```
aiebu-asm -t <target> [target-specific options]
```

### ASM Global Options

```
-t, --target TARGET    Select target architecture (required)
-v, --version          Print version and exit
-h, --help             Show this help message and exit
```

To see per-target options, run:

```
aiebu-asm -t <target> --help
```

---

### Target: aie2txn

Assembles a **transaction binary** (`.bin`) into an ELF. This is the most common workflow for AIE2 devices.

```
aiebu-asm -t aie2txn -c <txn.bin> -o <output.elf> [options]
```

**Options:**

| Flag | Description |
|------|-------------|
| `-c, --controlcode FILE` | Transaction control code binary (required) |
| `-o, --outputelf FILE` | Output ELF file (required) |
| `-p, --controlpkt FILE` | Control packet binary |
| `-j, --json FILE` | Patch JSON file (`external_buffer_id.json`) |
| `-l, --lib LIB` | Library to link (repeatable) |
| `-L, --libpath PATH` | Library search directory (repeatable) |
| `-m, --pmctrl ID:FILE` | PM control packet: integer ID and file path (repeatable) |
| `-r, --report` | Print a disassembly report to stdout after assembling |
| `-h, --help` | Show help |

**Examples:**

```bash
# Minimal: transaction binary only
aiebu-asm -t aie2txn \
    -c build/ml_txn.bin \
    -o output.elf

# With control packet and patch JSON
aiebu-asm -t aie2txn \
    -c build/ml_txn.bin \
    -p build/ctrl_pkt0.bin \
    -j build/external_buffer_id.json \
    -o output.elf

# With PM control packet (ID=0) and a second PM packet (ID=1)
aiebu-asm -t aie2txn \
    -c build/ml_txn.bin \
    -p build/ctrl_pkt0.bin \
    -j build/external_buffer_id.json \
    -m 0:build/pm_ctrl0.bin \
    -m 1:build/pm_ctrl1.bin \
    -o output.elf

# Assemble and print report + disassembly
aiebu-asm -t aie2txn -r \
    -c build/ml_txn.bin \
    -p build/ctrl_pkt0.bin \
    -j build/external_buffer_id.json \
    -o output.elf

# With linked libraries
aiebu-asm -t aie2txn \
    -c build/ml_txn.bin \
    -L /opt/aiebu/lib -l libaie_common \
    -o output.elf
```

---

### Target: aie2dpu

Assembles a **DPU control code binary** into an ELF. Same options as `aie2txn` minus PM control packets.

```
aiebu-asm -t aie2dpu -c <dpu.bin> -o <output.elf> [options]
```

**Examples:**

```bash
aiebu-asm -t aie2dpu \
    -c build/dpu_ctrl.bin \
    -o output.elf

aiebu-asm -t aie2dpu \
    -c build/dpu_ctrl.bin \
    -p build/ctrl_pkt0.bin \
    -j build/external_buffer_id.json \
    -o output.elf
```

---

### Target: aie2asm

Assembles **AIE2 assembly text** into an ELF. Accepts the same options as `aie2txn`.

```
aiebu-asm -t aie2asm -c <source.asm> -o <output.elf> [options]
```

**Example:**

```bash
aiebu-asm -t aie2asm \
    -c src/kernel.asm \
    -L src/include \
    -o output.elf
```

---

### Target: aie2ps

Assembles **AIE2PS assembly text** into an ELF.

```
aiebu-asm -t aie2ps -c <source.asm> -o <output.elf> [options]
```

**Options:**

| Flag | Description |
|------|-------------|
| `-c, --asm FILE` | ASM source file (required) |
| `-o, --outputelf FILE` | Output ELF file (required) |
| `-j, --json FILE` | Patch JSON file |
| `-L, --libpath PATH` | Directory to search for included files and libs (repeatable) |
| `-f, --flag FLAG` | Assembly flags (repeatable; see below) |
| `-h, --help` | Show help |

**Flags:**

| Flag | Effect |
|------|--------|
| `disabledump` | Suppress the `.dump` debug section in the output ELF |
| `fulldump` | Include full debug information in the `.dump` section |
| `loglevel_error` | Only log errors |
| `loglevel_warn` | Log warnings and errors |
| `loglevel_info` | Log informational messages |
| `loglevel_debug` | Log all debug output |

**Examples:**

```bash
# Basic assembly
aiebu-asm -t aie2ps \
    -c src/control.asm \
    -o output.elf

# With library path (for .include resolution)
aiebu-asm -t aie2ps \
    -c src/control.asm \
    -L src/include \
    -L /opt/aiebu/lib/aie2ps \
    -o output.elf

# Suppress debug dump section
aiebu-asm -t aie2ps \
    -c src/control.asm \
    -f disabledump \
    -o output.elf

# With patch JSON and debug logging
aiebu-asm -t aie2ps \
    -c src/control.asm \
    -j src/external_buffer_id.json \
    -f loglevel_debug \
    -o output.elf
```

---

### Target: aie4

Assembles **AIE4 assembly text** into an ELF. The specific AIE4 variant (aie4, aie4a, aie4z) is determined automatically from a `.target` directive inside the ASM file.

```
aiebu-asm -t aie4 -c <source.asm> -o <output.elf> [options]
```

**Options:** Same as `aie2ps` (`-c`, `-o`, `-j`, `-L`, `-f`, `-h`).

**Examples:**

```bash
aiebu-asm -t aie4 \
    -c src/compute.asm \
    -o output.elf

aiebu-asm -t aie4 \
    -c src/compute.asm \
    -L src/include \
    -f disabledump \
    -o output.elf
```

---

### Target: aie2_config

Assembles a **full AIE2 config ELF** driven by a JSON descriptor.

```
aiebu-asm -t aie2_config -j <config.json> -o <output.elf> [options]
```

**Options:**

| Flag | Description |
|------|-------------|
| `-j, --json FILE` | Config JSON descriptor file |
| `-o, --outputelf FILE` | Output ELF file (required) |
| `-f, --flag FLAG` | Assembly flags (repeatable) |
| `-h, --help` | Show help |

The JSON file's **parent directory** is automatically added to the library search path so that files referenced inside the JSON are found relative to it.

**Example:**

```bash
aiebu-asm -t aie2_config \
    -j configs/aie2/config.json \
    -o output.elf
```

---

### Target: aie2ps_config

Assembles a **full AIE2PS config ELF**. Supports an optimization level flag.

```
aiebu-asm -t aie2ps_config -j <config.json> -o <output.elf> [options]
```

**Options:**

| Flag | Description |
|------|-------------|
| `-j, --json FILE` | Config JSON descriptor file |
| `-o, --outputelf FILE` | Output ELF file (required) |
| `-f, --flag FLAG` | Assembly flags: `disabledump`, `fulldump`, `loglevel_*` (repeatable) |
| `-O, --optimization LEVEL` | Optimization level 1–4 (default: 0, no optimization) |
| `-h, --help` | Show help |

**Examples:**

```bash
# Basic config ELF
aiebu-asm -t aie2ps_config \
    -j configs/aie2ps/config.json \
    -o output.elf

# With optimization level 2 and no debug dump
aiebu-asm -t aie2ps_config \
    -j configs/aie2ps/config.json \
    -f disabledump \
    -O 2 \
    -o output.elf
```

---

### Target: aie4_config

Assembles a **full AIE4 config ELF**. The specific AIE4 variant is determined by a `.target` directive in the referenced ASM files.

```
aiebu-asm -t aie4_config -j <config.json> -o <output.elf> [options]
```

**Options:** Same as `aie2ps_config` (`-j`, `-o`, `-f`, `-O`, `-h`).

**Examples:**

```bash
aiebu-asm -t aie4_config \
    -j configs/aie4/config.json \
    -o output.elf

aiebu-asm -t aie4_config \
    -j configs/aie4/config.json \
    -O 1 \
    -f loglevel_warn \
    -o output.elf
```

---

## aiebu-dump

`aiebu-dump` inspects AIE ELF files and binary blobs. It auto-detects the file type from the ELF header; use `-m` to specify the architecture when giving it a raw binary.

```
aiebu-dump [options] <filename>
```

### Dump Options

| Flag | Description |
|------|-------------|
| `-a, --archive-headers` | Display archive header information |
| `-f, --file-headers` | Display ELF file header |
| `-x, --all-headers` | Display all ELF headers and opcode frequency (AIE2 ELF only) |
| `-d, --disassemble` | Disassemble `.ctrltext` section (ELF) or display control packet format (binary) |
| `-D, --disassemble-all` | Disassemble all sections |
| `-t, --syms` | Display symbol table |
| `-r, --reloc` | Display relocation entries |
| `-p, --private-headers` | Display opcode frequency in control code |
| `-m, --architecture ARCH` | Specify target architecture for binary files: `aie2ps`, `aie4`, `aie2asm`, `aie2txn`, `aie2dpu` |
| `-P, --private ARG` | Display private debug data: `trace-probe` or `opcode-info` |
| `--pc VALUE` | Program counter for `opcode-info` lookup (hex or decimal) |
| `--page-index VALUE` | Page index for `opcode-info` lookup |
| `--uc-index VALUE` | Microcontroller (column) index filter for `opcode-info` |
| `-H, --help` | Show help |
| `-v, --version` | Show version |

### Dump Usage Examples

**Display all ELF headers and opcode frequency (AIE2 ELF):**

```bash
aiebu-dump -x output.elf
```

**Disassemble the control text section of an ELF:**

```bash
aiebu-dump -d output.elf
```

**Disassemble all sections of an ELF:**

```bash
aiebu-dump -D output.elf
```

**Display symbol table:**

```bash
aiebu-dump -t output.elf
```

**Display relocation entries:**

```bash
aiebu-dump -r output.elf
```

**Display opcode frequency from a binary (architecture must be specified):**

```bash
aiebu-dump -p -m aie2ps raw_binary.bin
```

**Disassemble a raw binary file (AIE2PS):**

```bash
aiebu-dump -d -m aie2ps raw_binary.bin
```

**Disassemble a raw binary file (AIE4):**

```bash
aiebu-dump -d -m aie4 raw_binary.bin
```

**Display trace probe information from `.dump` section:**

```bash
aiebu-dump -P trace-probe output.elf
```

The file must be an AIE2PS or AIE4 ELF (non-binary) assembled without `disabledump`.

**Display opcode information by PC and page index:**

```bash
aiebu-dump -P opcode-info --pc 0x48 --page-index 0 output.elf
```

**Filter opcode-info to a specific column:**

```bash
aiebu-dump -P opcode-info --pc 0x48 --page-index 0 --uc-index 2 output.elf
```

**Disassemble a control packet binary:**

```bash
aiebu-dump -d ctrl_pkt0.bin
```

When no `-m` flag is given for a binary file, `aiebu-dump` warns and defaults to `aie2ps`.

---

## ELF Output Sections

The following sections may appear in the output ELF depending on the input and options.

| Section | Description |
|---------|-------------|
| `.ctrltext` | Transaction / instruction control code binary |
| `.ctrldata` | Control packet binary |
| `.preempt_save` | Control code for save operation during preemption |
| `.preempt_restore` | Control code for restore operation during preemption |
| `.ctrlpkt.pm.N` | PM control packet with ID `N` |
| `.dynsym` | Dynamic symbol table |
| `.dynstr` | Dynamic symbol string table |
| `.dynamic` | Dynamic relocation pointers and sizes |
| `.reldyn` | Dynamic relocation entries |
| `.dump` | Debug JSON with opcode-to-source-line mappings (suppressed by `disabledump`) |

---

## Common Workflows

### Workflow 1 — Transaction binary to ELF (CLI)

```bash
# 1. Prepare inputs
#    ml_txn.bin           — transaction binary from model compiler
#    ctrl_pkt0.bin        — control packet binary
#    external_buffer_id.json — patch descriptor

# 2. Assemble
aiebu-asm -t aie2txn \
    -c ml_txn.bin \
    -p ctrl_pkt0.bin \
    -j external_buffer_id.json \
    -o output.elf

# 3. Inspect
aiebu-dump -d output.elf
aiebu-dump -t output.elf
```

### Workflow 2 — Transaction binary to ELF (C++ API)

```cpp
#include "aiebu/aiebu_assembler.h"
#include "aiebu/aiebu_error.h"
#include <fstream>

std::vector<char> read_file(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(f), {}};
}

int main() {
    auto txn   = read_file("ml_txn.bin");
    auto cpkt  = read_file("ctrl_pkt0.bin");
    auto patch = read_file("external_buffer_id.json");

    try {
        aiebu::aiebu_assembler as(
            aiebu::aiebu_assembler::buffer_type::blob_instr_transaction,
            txn, cpkt, patch);

        auto elf = as.get_elf();
        std::ofstream out("output.elf", std::ios::binary);
        out.write(elf.data(), elf.size());
    }
    catch (const aiebu::error& ex) {
        std::cerr << "error " << ex.get_code() << ": " << ex.what() << "\n";
        return 1;
    }
}
```

### Workflow 3 — Config ELF on disk (CLI)

```bash
# Directory layout:
#   configs/aie4/
#     config.json
#     kernel.asm
#     pdi.asm
#     ctrlpkt.asm
#     ctrlpkt0.dat
#     external_buffer_id.json

aiebu-asm -t aie4_config \
    -j configs/aie4/config.json \
    -o output.elf

aiebu-dump -d output.elf
```

### Workflow 4 — Config ELF fully in-memory (C++ API)

```cpp
aiebu::file_artifact resolver;
resolver.add_vfile("kernel.asm",              read_file("kernel.asm"));
resolver.add_vfile("pdi.asm",                 read_file("pdi.asm"));
resolver.add_vfile("ctrlpkt.asm",             read_file("ctrlpkt.asm"));
resolver.add_vfile("ctrlpkt0.dat",            read_file("ctrlpkt0.dat"));
resolver.add_vfile("external_buffer_id.json", read_file("external_buffer_id.json"));

auto config_json = read_file("config.json");

aiebu::aiebu_assembler as(
    aiebu::aiebu_assembler::buffer_type::aie4_config,
    config_json,
    resolver,
    {}   // no flags
);

auto elf = as.get_elf();
```

### Workflow 5 — XRT argument remapping (C++ API)

```cpp
// Load existing ELF
aiebu::aiebu_assembler as("kernel.elf");

// Get the argument table for kernel "DPU"
auto tbl  = as.get_argtbl("DPU");
auto& row = tbl.get()[0].inst_arginfo;  // instance 0

// Change buffer index 3 to point to a different XRT buffer
row[3].xrt_idx = 7;

// Rename and flush
tbl.set_name("CONV");
as.flush_argtbl(tbl);

auto elf_new = as.get_elf();
std::ofstream out("kernel_remapped.elf", std::ios::binary);
out.write(elf_new.data(), elf_new.size());
```

### Workflow 6 — Query opcode source locations (C++ API)

```cpp
// Assemble without disabledump (required for .dump section)
aiebu::aiebu_assembler as_build(
    aiebu::aiebu_assembler::buffer_type::aie4_config,
    {}, {}, {"configs/aie4"}, config_json);

// Re-load the ELF for querying
auto elf_data = as_build.get_elf();
aiebu::aiebu_assembler as(elf_data);

// Find all SAVE_TIMESTAMPS (0x1c) in kernel "DPU"
auto tbl = as.get_op_locations(0x1c, "DPU");
for (const auto& loc : tbl.get_line_info()) {
    for (const auto& li : loc.line_info) {
        std::cout << "col=" << li.col << "\n";
        for (const auto& [line, file] : li.entries)
            std::cout << "  " << file << ":" << line << "\n";
    }
}
```
