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

#include "mouse.h"

// Struct for binary settings data
typedef struct settings_bin {
    uint8_t bytes[8];
    size_t size;
} settings_bin_t;

// Can't use malloc in embedded so need static alloc
// extern settings_bin_t binary_settings; // Global binary representation of settings // REMOVE

void settings_encode(settings_bin_t *binary_settings, mouse_opts_t *options);
bool settings_decode(settings_bin_t *binary_settings, mouse_opts_t *options);
