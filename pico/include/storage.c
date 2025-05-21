/*
 * Anachro Mouse, a usb to serial mouse adaptor. Copyright (C) 2025 Aviancer <oss+amouse@skyvian.me>
 *
 * This library is free software; you can redistribute it and/or modify it under the terms of the 
 * GNU Lesser General Public License as published by the Free Software Foundation; either version 
 * 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without 
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with this library; 
 * if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
*/

#include "pico/flash.h"
#include "hardware/flash.h"

#include "../shared/crc8/crc8.h"

/* 
   Application code lives on the same flash space, and is always programmed to the front of the flash.
   We should ensure we write our data starting from the end of the flash. There is about 2MB of space.

   PICO_FLASH_SIZE_BYTES | 2MB        | Total size of flash
   FLASH_SECTOR_SIZE     | 4096 bytes | Single sector - minimum size for erasure.
   FLASH_PAGE_SIZE       | 256 bytes  | Single page - minimum size for writing data.

   We use flash_safe_execute() to call flash functions inside safe context.
   This avoids interrupts and running other code, including interacting with flash on the second core.
*/

// Address for last available sector
#define FLASH_TARGET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)

// Pointer to flash storage area
// Should likely be a const, but causes issues with interface compatibility for Linux currently
uint8_t *flash_target_contents = (uint8_t *) (XIP_BASE + FLASH_TARGET);

// This function will be called when it's safe to call flash_range_erase
static void call_flash_range_erase(void *param) {
   uint32_t offset = (uint32_t)param;
   flash_range_erase(offset, FLASH_SECTOR_SIZE);
}

// This function will be called when it's safe to call flash_range_program
static void call_flash_range_program(void *param) {
   uint32_t offset = ((uintptr_t*)param)[0];
   const uint8_t *data = (const uint8_t *)((uintptr_t*)param)[1];
   size_t size = ((uintptr_t*)param)[2];

   flash_range_program(offset, data, size);
}

static void erase_flash_settings() {
   // timeout = UINT32_MAX
   int rc = flash_safe_execute(call_flash_range_erase, (void*)FLASH_TARGET, UINT32_MAX);
   hard_assert(rc == PICO_OK);
}

uint8_t* ptr_flash_settings() {
   return flash_target_contents;
}

void write_flash_settings(uint8_t *buffer, size_t size) {
   erase_flash_settings(); // Erase is implicit in writing

   uintptr_t params[] = { FLASH_TARGET, (uintptr_t)buffer, size };
   // timeout = UINT32_MAX
   int rc = flash_safe_execute(call_flash_range_program, params, UINT32_MAX);
   hard_assert(rc == PICO_OK);
}
