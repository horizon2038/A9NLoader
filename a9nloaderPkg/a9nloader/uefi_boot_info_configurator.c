#include "uefi_boot_info_configurator.h"

#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Uefi.h>

#include "acpi.h"
#include "boot_info.h"
#include "memory_type.h"
#include "stdint.h"
#include "uefi_memory_map.h"

static void add_or_merge_entry(
    boot_info         *target_boot_info,
    memory_map_entry **last_entry_ptr,
    memory_map_entry   new_entry
)
{
    memory_map_entry *last_entry = *last_entry_ptr;

    if (new_entry.page_count == 0)
    {
        return;
    }

    if (last_entry && last_entry->type == new_entry.type
        && (last_entry->physical_address_start + (last_entry->page_count * EFI_PAGE_SIZE)
            == new_entry.physical_address_start))
    {
        last_entry->page_count += new_entry.page_count;
    }
    else
    {
        memory_map_entry *current_entry
            = &target_boot_info->boot_memory_info
                   .memory_map[target_boot_info->boot_memory_info.memory_map_count++];
        *current_entry  = new_entry;
        *last_entry_ptr = current_entry;
    }
}

EFI_STATUS make_boot_info(
    EFI_SYSTEM_TABLE *system_table,
    uefi_memory_map  *target_uefi_memory_map,
    boot_info        *target_boot_info
)
{
    if (!target_boot_info->boot_memory_info.memory_map)
    {
        return EFI_NOT_FOUND;
    }

    if (target_uefi_memory_map->descriptor_size == 0)
    {
        return EFI_INVALID_PARAMETER;
    }

    {
        UINTN count = target_uefi_memory_map->map_size / target_uefi_memory_map->descriptor_size;
        void *temp  = NULL;
        EFI_STATUS status
            = gBS->AllocatePool(EfiLoaderData, target_uefi_memory_map->descriptor_size, &temp);
        if (EFI_ERROR(status))
        {
            return status;
        }

        for (UINTN i = 0; i < count; ++i)
        {
            for (UINTN j = 0; j < count - 1 - i; ++j)
            {
                EFI_MEMORY_DESCRIPTOR *current
                    = (EFI_MEMORY_DESCRIPTOR *)((CHAR8 *)target_uefi_memory_map->buffer
                                                + j * target_uefi_memory_map->descriptor_size);
                EFI_MEMORY_DESCRIPTOR *next
                    = (EFI_MEMORY_DESCRIPTOR *)((CHAR8 *)target_uefi_memory_map->buffer
                                                + (j + 1) * target_uefi_memory_map->descriptor_size);

                if (current->PhysicalStart > next->PhysicalStart)
                {
                    CopyMem(temp, current, target_uefi_memory_map->descriptor_size);
                    CopyMem(current, next, target_uefi_memory_map->descriptor_size);
                    CopyMem(next, temp, target_uefi_memory_map->descriptor_size);
                }
            }
        }
        gBS->FreePool(temp);
    }

    for (uintmax_t i = 0; i < 8; i++)
    {
        target_boot_info->arch_info[i] = 0xdeadbeaf;
    }
    target_boot_info->arch_info[0] = (uintmax_t)(find_rsdp(system_table));

    UINTN entries_count = target_uefi_memory_map->map_size / target_uefi_memory_map->descriptor_size;
    target_boot_info->boot_memory_info.memory_map_count = 0;
    target_boot_info->boot_memory_info.memory_size      = 0;
    memory_map_entry *last_entry                        = NULL;
    uint64_t          last_processed_addr               = 0;

    for (UINTN i = 0; i < entries_count; i++)
    {
        EFI_MEMORY_DESCRIPTOR *uefi_desc
            = (EFI_MEMORY_DESCRIPTOR *)((CHAR8 *)target_uefi_memory_map->buffer
                                        + i * target_uefi_memory_map->descriptor_size);

        if (uefi_desc->NumberOfPages == 0)
        {
            continue;
        }

        if (uefi_desc->PhysicalStart > last_processed_addr)
        {
            memory_map_entry gap_entry;
            gap_entry.physical_address_start = last_processed_addr;
            gap_entry.page_count = (uefi_desc->PhysicalStart - last_processed_addr) / EFI_PAGE_SIZE;
            gap_entry.type       = DEVICE_MEMORY;
            add_or_merge_entry(target_boot_info, &last_entry, gap_entry);
        }

        memory_map_entry new_entry;
        new_entry.physical_address_start = uefi_desc->PhysicalStart;
        new_entry.page_count             = uefi_desc->NumberOfPages;

        switch (uefi_desc->Type)
        {
            case EfiBootServicesCode :
            case EfiBootServicesData :
            case EfiConventionalMemory :
            case EfiACPIReclaimMemory :
            case EfiPersistentMemory :
                new_entry.type = FREE_MEMORY;
                break;
            case EfiReservedMemoryType :
            case EfiRuntimeServicesCode :
            case EfiRuntimeServicesData :
            case EfiUnusableMemory :
            case EfiACPIMemoryNVS :
            case EfiPalCode :
                new_entry.type = RESERVED_MEMORY;
                break;
            case EfiLoaderCode :
            case EfiLoaderData :
            case EfiMemoryMappedIO :
            case EfiMemoryMappedIOPortSpace :
            default :
                new_entry.type = DEVICE_MEMORY;
                break;
        }

        add_or_merge_entry(target_boot_info, &last_entry, new_entry);

        last_processed_addr = uefi_desc->PhysicalStart + (uefi_desc->NumberOfPages * EFI_PAGE_SIZE);
    }

    const uint64_t max_address = (uint64_t)1 << 46;
    if (last_processed_addr < max_address)
    {
        memory_map_entry final_gap_entry;
        final_gap_entry.physical_address_start = last_processed_addr;
        final_gap_entry.page_count = (max_address - last_processed_addr) / EFI_PAGE_SIZE;
        final_gap_entry.type       = DEVICE_MEMORY;
        add_or_merge_entry(target_boot_info, &last_entry, final_gap_entry);
    }

    target_boot_info->boot_memory_info.memory_size = 0;
    for (UINTN i = 0; i < target_boot_info->boot_memory_info.memory_map_count; i++)
    {
        memory_map_entry *entry = &target_boot_info->boot_memory_info.memory_map[i];
        target_boot_info->boot_memory_info.memory_size += entry->page_count * EFI_PAGE_SIZE;
    }

    return EFI_SUCCESS;
}
