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

#ifdef __linux__
#include <stdlib.h>
#else 
#include "pico/stdlib.h"
#endif

#include "crc8/crc8.h"
#include "mouse.h"
#include "settings.h"

#define SETTINGS_VERSION 0x00
#define SETTINGS_SIZE 8

#define FLASH_OPT1_BYTE 3
#define FLASH_OPT2_BYTE 4
#define FLASH_CRC_BYTE 7

// Can't use malloc in embedded so need static alloc
settings_bin_t binary_settings; // Global updatable binary representation of settings

bool assert_byte(uint8_t byte1, uint8_t byte2) {
    return(byte1 == byte2);
}

bool settings_decode(uint8_t *binary, mouse_opts_t *options) {

    // CRC check
    if(binary[FLASH_CRC_BYTE] != crc8(&binary[0], 7, (uint8_t)0x00)) {
        // CRC FAILURE
        return false;
    }

    bool state = true;

    state &= assert_byte(0x4D, binary[0]);
    state &= assert_byte(0x6F, binary[1]);
    state &= assert_byte(SETTINGS_VERSION, binary[2]);
    uint8_t settings1 = binary[FLASH_OPT1_BYTE];
    uint8_t settings2 = binary[FLASH_OPT2_BYTE];
    state &= assert_byte(0x75, binary[5]);
    state &= assert_byte(0x53, binary[6]);

    if(!state) { return false; }

    uint8_t sens;
    options->protocol = clampi(settings1 & 0x03, 0, 3);        // 0x03 == 0b11
    sens = (settings1 >> 2) & 0x0F;                            // Shifting right to get rid of proto, 0x0F == 0b1111
    options->sensitivity  = clampf(sens * 0.2, 0.2, 2.5);
    options->wheel        = (bool)(settings1 >> 6) & 0x01;    // Shifting right to get rid of proto and sensitivity, 0x01 is a bool
    options->swap_buttons = (bool)(settings1 >> 7) & 0x01;    // Shifting right to get rid of proto, sensitivity, and the first flag, 0x01 is a bool 

    return state;
}

void settings_encode() {

    /* Magic bytes?
     *  Easy option? Convert to bitmask?
     *  
     *  Minimum page size is a 256 byte write.
     *
     *  Layout:
     *    [canary][version][options][canary][checksum] (crc-8?)
     *
     *    Mo[version][options]uS[checksum]
     *
     *  Version: 1 byte, 0x00-0xFF
     *
     *  Bits
     *    How many protocols do we want to support? At least 3, probably 4.
     *  
     *    Sensitivity is between 0.2 and 2.5 with increments/decrements of 0.2.
     *    Effectively 12.5 values. So 4 bits (for 16 values)
     *
     *    Flags: (0x06) WHEEL, (0x07) SWAP_BUTTONS
     *
     *  |               RESERVED|FLAGS|SENSITIVITY|PROTO|
     *  |15 14 13 12 11 10 09 08|07 06|05 04 03 02|01 00|
     *
    */

    binary_settings.size = SETTINGS_SIZE;

    binary_settings.bytes[0] = 0x4D;             // 00: Canary M
    binary_settings.bytes[1] = 0x6F;             // 01: Canary o
    binary_settings.bytes[2] = SETTINGS_VERSION; // 02: Config version 0x00
    binary_settings.bytes[3] = 0x00;             // 03: Options 1
    binary_settings.bytes[4] = 0x00;             // 04: Options 2 (Reserved)
    binary_settings.bytes[5] = 0x75;             // 05: Canary u
    binary_settings.bytes[6] = 0x53;             // 06: Canary S
    binary_settings.bytes[7] = 0x00;             // 07: CRC-8 Checksum
 
    // Writing options
    // Convert mouse options struct into bitfield that can be written to flash
 
    // Approximal conversion from float to 0.2 multiplier
    uint8_t sensitivity = clampi((int)(mouse_options.sensitivity / 0.2), 1, 16);
 
    binary_settings.bytes[FLASH_OPT1_BYTE] = 
       ((mouse_options.protocol    & 0x03) | 
       (sensitivity                & 0x0F) << 2 | 
       (mouse_options.wheel        & 0x01) << 6 |
       (mouse_options.swap_buttons & 0x01) << 7);
 
    // Calculate CRC of configuration data and store it alongside it
    binary_settings.bytes[FLASH_CRC_BYTE] = crc8(&binary_settings.bytes[0], 7, (uint8_t)0x00);

}
 