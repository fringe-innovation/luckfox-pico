/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2022 Rockchip Electronics Co., Ltd.
 */
#include "soc.h"

#if defined(HAL_AP_CORE) && defined(HAL_DCACHE_MODULE_ENABLED)
static uint32_t Sect_Normal;        // outer & inner wb/wa, non-shareable, executable, rw, domain 0
static uint32_t Sect_Normal_SH;     // as Sect_Normal, but shareable
static uint32_t Sect_Device_RW;     // as Sect_Device_RO, but writeable
static uint32_t Sect_Normal_NC;     // Outer & inner non-cacheable, non-shareable, executable, rw, domain 0
static uint32_t Sect_Normal_NC_SH;  // as Sect_Normal_NC, but shareable

extern uint32_t MMUTable[];
void MMU_CreateTranslationTable(void)
{
    mmu_region_attributes_Type region;

    // Create 4GB of faulting entries
    MMU_TTSection(MMUTable, 0, 4096, DESCRIPTOR_FAULT);

    /*
     * Generate descriptors. Refer to core_ca.h to get information about attributes
     *
     */
    // Create descriptors for Vectors, RO, RW, ZI sections
    section_normal(Sect_Normal, region);
    region.sh_t = SHARED;
    MMU_GetSectionDescriptor(&Sect_Normal_SH, region);
    // Create descriptors for peripherals
    section_device_rw(Sect_Device_RW, region);
    // Create descriptors for Uncache Memory
    section_normal_nc(Sect_Normal_NC, region);
    region.sh_t = SHARED;
    MMU_GetSectionDescriptor(&Sect_Normal_NC_SH, region);

    /*
     *  Define MMU flat-map regions and attributes
     *
     */
    // Define dram address space
    MMU_TTSection(MMUTable, FIRMWARE_BASE, DRAM_SIZE >> 20, Sect_Normal);
    MMU_TTSection(MMUTable, SHMEM_BASE, SHMEM_SIZE >> 20, Sect_Normal_SH);
//    MMU_TTSection(MMUTable, SHMEM_BASE, SHMEM_SIZE >> 20, Sect_Normal_NC_SH);
#ifdef LINUX_RPMSG_BASE
    MMU_TTSection(MMUTable, LINUX_RPMSG_BASE, LINUX_RPMSG_SIZE >> 20, Sect_Normal_NC_SH);
#endif

    //--------------------- PERIPHERALS -------------------
    MMU_TTSection(MMUTable, 0xF0000000, 233U, Sect_Device_RW);

    /* Set location of level 1 page table
    ; 31:14 - Translation table base addr (31:14-TTBCR.N, TTBCR.N is 0 out of reset)
    ; 13:7  - 0x0
    ; 6     - IRGN[0] 0x0 (Inner WB WA)
    ; 5     - NOS     0x0 (Non-shared)
    ; 4:3   - RGN     0x1 (Outer WB WA)
    ; 2     - IMP     0x0 (Implementation Defined)
    ; 1     - S       0x0 (Non-shared)
    ; 0     - IRGN[1] 0x1 (Inner WB WA) */
    __set_TTBR0(((uint32_t)MMUTable) | 9U);
    __ISB();

    /* Set up domain access control register
    ; We set domain 0 to Client and all other domains to No Access.
    ; All translation table entries specify domain 0 */
    __set_DACR(1);
    __ISB();
}
#endif
