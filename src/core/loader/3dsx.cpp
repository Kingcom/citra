// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <vector>

#include "core/file_sys/archive_romfs.h"
#include "core/loader/elf.h"
#include "core/loader/ncch.h"
#include "core/hle/service/fs/archive.h"
#include "core/mem_map.h"

#include "3dsx.h"


namespace Loader {


/** 
 * File layout:
 * - File header
 * - Code, rodata and data relocation table headers
 * - Code segment
 * - Rodata segment
 * - Loadable (non-BSS) part of the data segment
 * - Code relocation table
 * - Rodata relocation table
 * - Data relocation table
 *
 * Memory layout before relocations are applied:
 * [0..codeSegSize)             -> code segment
 * [codeSegSize..rodataSegSize) -> rodata segment
 * [rodataSegSize..dataSegSize) -> data segment
 *
 * Memory layout after relocations are applied: well, however the loader sets it up :)
 * The entrypoint is always the start of the code segment.
 * The BSS section must be cleared manually by the application.
 */
enum THREEDSX_Error {
    ERROR_NONE = 0,
    ERROR_READ = 1,
    ERROR_FILE = 2,
    ERROR_ALLOC = 3
};
static const u32 RELOCBUFSIZE = 512;

// File header
static const u32 THREEDSX_MAGIC = 0x58534433; // '3DSX'
#pragma pack(1)
struct THREEDSX_Header
{
    u32 magic;
    u16 header_size, reloc_hdr_size;
    u32 format_ver;
    u32 flags;

    // Sizes of the code, rodata and data segments +
    // size of the BSS section (uninitialized latter half of the data segment)
    u32 code_seg_size, rodata_seg_size, data_seg_size, bss_size;
};

// Relocation header: all fields (even extra unknown fields) are guaranteed to be relocation counts.
struct THREEDSX_RelocHdr
{
    // # of absolute relocations (that is, fix address to post-relocation memory layout)
    u32 cross_segment_absolute; 
    // # of cross-segment relative relocations (that is, 32bit signed offsets that need to be patched)
    u32 cross_segment_relative; 
    // more?

    // Relocations are written in this order:
    // - Absolute relocations
    // - Relative relocations
};

// Relocation entry: from the current pointer, skip X words and patch Y words
struct THREEDSX_Reloc
{
    u16 skip, patch;
};
#pragma pack()

struct THREEloadinfo
{
    u8* seg_ptrs[3]; // code, rodata & data
    u32 seg_addrs[3];
    u32 seg_sizes[3];
};

class THREEDSXReader {
public:
     static int Load3DSXFile(const std::string& filename, u32 base_addr);
};

static u32 TranslateAddr(u32 addr, THREEloadinfo *loadinfo, u32* offsets)
{
    if (addr < offsets[0])
        return loadinfo->seg_addrs[0] + addr;
    if (addr < offsets[1])
        return loadinfo->seg_addrs[1] + addr - offsets[0];
    return loadinfo->seg_addrs[2] + addr - offsets[1];
}

int THREEDSXReader::Load3DSXFile(const std::string& filename, u32 base_addr)
{
    FileUtil::IOFile file(filename, "rb");
    if (!file.IsOpen()) {
        return ERROR_FILE;
    }
    THREEDSX_Header hdr;
    if (file.ReadBytes(&hdr, sizeof(hdr)) != sizeof(hdr))
        return ERROR_READ;

    THREEloadinfo loadinfo;
    //loadinfo segments must be a multiple of 0x1000
    loadinfo.seg_sizes[0] = (hdr.code_seg_size + 0xFFF) &~0xFFF;
    loadinfo.seg_sizes[1] = (hdr.rodata_seg_size + 0xFFF) &~0xFFF;
    loadinfo.seg_sizes[2] = (hdr.data_seg_size + 0xFFF) &~0xFFF;
    u32 offsets[2] = { loadinfo.seg_sizes[0], loadinfo.seg_sizes[0] + loadinfo.seg_sizes[1] };
    u32 data_load_size = (hdr.data_seg_size - hdr.bss_size + 0xFFF) &~0xFFF;
    u32 bss_load_size = loadinfo.seg_sizes[2] - data_load_size;
    u32 n_reloc_tables = hdr.reloc_hdr_size / 4;
    std::vector<u8> all_mem(loadinfo.seg_sizes[0] + loadinfo.seg_sizes[1] + loadinfo.seg_sizes[2] + 3 * n_reloc_tables);

    loadinfo.seg_addrs[0] = base_addr;
    loadinfo.seg_addrs[1] = loadinfo.seg_addrs[0] + loadinfo.seg_sizes[0];
    loadinfo.seg_addrs[2] = loadinfo.seg_addrs[1] + loadinfo.seg_sizes[1];
    loadinfo.seg_ptrs[0] = &all_mem[0];
    loadinfo.seg_ptrs[1] = loadinfo.seg_ptrs[0] + loadinfo.seg_sizes[0];
    loadinfo.seg_ptrs[2] = loadinfo.seg_ptrs[1] + loadinfo.seg_sizes[1];

    // Skip header for future compatibility
    file.Seek(hdr.header_size, SEEK_SET);

    // Read the relocation headers
    u32* relocs = (u32*)(loadinfo.seg_ptrs[2] + hdr.data_seg_size);

    for (u32 current_segment = 0; current_segment < 3; current_segment++) {
        if (file.ReadBytes(&relocs[current_segment*n_reloc_tables], n_reloc_tables * 4) != n_reloc_tables * 4)
            return ERROR_READ;
    }

    // Read the segments
    if (file.ReadBytes(loadinfo.seg_ptrs[0], hdr.code_seg_size) != hdr.code_seg_size)
        return ERROR_READ;
    if (file.ReadBytes(loadinfo.seg_ptrs[1], hdr.rodata_seg_size) != hdr.rodata_seg_size)
        return ERROR_READ;
    if (file.ReadBytes(loadinfo.seg_ptrs[2], hdr.data_seg_size - hdr.bss_size) != hdr.data_seg_size - hdr.bss_size)
        return ERROR_READ;

    // BSS clear
    memset((char*)loadinfo.seg_ptrs[2] + hdr.data_seg_size - hdr.bss_size, 0, hdr.bss_size);

    // Relocate the segments
    for (u32 current_segment = 0; current_segment < 3; current_segment++) {
        for (u32 current_segment_reloc_table = 0; current_segment_reloc_table < n_reloc_tables; current_segment_reloc_table++) {
            u32 n_relocs = relocs[current_segment*n_reloc_tables + current_segment_reloc_table];
            if (current_segment_reloc_table >= 2) {
                // We are not using this table - ignore it because we don't know what it dose
                file.Seek(n_relocs*sizeof(THREEDSX_Reloc), SEEK_CUR);
                continue;
            }
            static THREEDSX_Reloc reloc_table[RELOCBUFSIZE];

            u32* pos = (u32*)loadinfo.seg_ptrs[current_segment];
            u32* end_pos = pos + (loadinfo.seg_sizes[current_segment] / 4);

            while (n_relocs) {
                u32 remaining = std::min(RELOCBUFSIZE, n_relocs);
                n_relocs -= remaining;

                if (file.ReadBytes(reloc_table, remaining*sizeof(THREEDSX_Reloc)) != remaining*sizeof(THREEDSX_Reloc))
                    return ERROR_READ;

                for (u32 current_inprogress = 0; current_inprogress < remaining && pos < end_pos; current_inprogress++) {
                    LOG_TRACE(Loader, "(t=%d,skip=%u,patch=%u)\n",
                        current_segment_reloc_table, (u32)reloc_table[current_inprogress].skip, (u32)reloc_table[current_inprogress].patch);
                    pos += reloc_table[current_inprogress].skip;
                    s32 num_patches = reloc_table[current_inprogress].patch;
                    while (0 < num_patches && pos < end_pos) {
                        u32 in_addr = (char*)pos - (char*)&all_mem[0];
                        u32 addr = TranslateAddr(*pos, &loadinfo, offsets);
                        LOG_TRACE(Loader, "Patching %08X <-- rel(%08X,%d) (%08X)\n",
                            base_addr + in_addr, addr, current_segment_reloc_table, *pos);
                        switch (current_segment_reloc_table) {
                        case 0: *pos = (addr); break;
                        case 1: *pos = (addr - in_addr); break;
                        default: break; //this should never happen
                        }
                        pos++;
                        num_patches--;
                    }
                }
            }
        }
    }

    // Write the data
    memcpy(Memory::GetPointer(base_addr), &all_mem[0], loadinfo.seg_sizes[0] + loadinfo.seg_sizes[1] + loadinfo.seg_sizes[2]);

    LOG_DEBUG(Loader, "CODE:   %u pages\n", loadinfo.seg_sizes[0] / 0x1000);
    LOG_DEBUG(Loader, "RODATA: %u pages\n", loadinfo.seg_sizes[1] / 0x1000);
    LOG_DEBUG(Loader, "DATA:   %u pages\n", data_load_size / 0x1000);
    LOG_DEBUG(Loader, "BSS:    %u pages\n", bss_load_size / 0x1000);

    return ERROR_NONE;
}

    /// AppLoader_DSX constructor
    AppLoader_THREEDSX::AppLoader_THREEDSX(const std::string& filename) : filename(filename) {
    }

    /// AppLoader_DSX destructor
    AppLoader_THREEDSX::~AppLoader_THREEDSX() {
    }

    /**
    * Loads a 3DSX file
    * @return Success on success, otherwise Error
    */
    ResultStatus AppLoader_THREEDSX::Load() {
        LOG_INFO(Loader, "Loading 3DSX file %s...", filename.c_str());
        FileUtil::IOFile file(filename, "rb");
        if (file.IsOpen()) {
            THREEDSXReader::Load3DSXFile(filename, 0x00100000);
            Kernel::LoadExec(0x00100000);
        } else {
            return ResultStatus::Error;
        }
        return ResultStatus::Success;
    }

} // namespace Loader
