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

#ifndef SETTINGS_H_
#define SETTINGS_H_

#include "mouse.h"

#define SETTINGS_VERSION 0x00
// Defined by size of a flash page on RP2040, effectively minimum writing size
// This has to be a multiple of 256 bytes (spi_flash.c)
#define SETTINGS_SIZE 256

void settings_encode(uint8_t *binary_settings, mouse_opts_t *options);
bool settings_decode(uint8_t *binary_settings, mouse_opts_t *options);

#endif // SETTINGS_H_
