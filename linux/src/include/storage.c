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
*/

/* Filesystem backed software flash memory layer for Linux, emulating Pico flash memory access */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pwd.h>
#include <limits.h>

#include "../../../shared/settings.h"

// Fake flash memory, emulating Pico flash access
uint8_t flash_storage_buffer[SETTINGS_SIZE] = {0};

void get_config_path(char* filepath) {
    struct passwd pwd;
    struct passwd* result;
    size_t pwdbuffer_size = sysconf(_SC_GETPW_R_SIZE_MAX);

    if(pwdbuffer_size == -1) {  // If not provided by OS
        pwdbuffer_size = 16384; // Plenty of margins
    }
    char pwdbuffer[pwdbuffer_size]; // At time of writing value was 70.

    uid_t uid = getuid();

    getpwuid_r(uid, &pwd, &pwdbuffer[0], sizeof(pwdbuffer), &result);
    if(result == NULL) {
        snprintf(filepath, PATH_MAX, "./.amouse.conf");
        fprintf(stderr, "Home dir lookup failed, using current dir(%s): %d: %s\n", filepath, errno, strerror(errno));
    }
    else {
        snprintf(filepath, PATH_MAX, "%s/%s", result->pw_dir, ".amouse.conf"); // Less portable
    }
}

// Filesystem backed data access, data is loaded to memory when pointer function is accessed
// Subsequent calls load any changed data from file. Returns NULL on error.
uint8_t* ptr_flash_settings() {
    char filepath[PATH_MAX] = {0};
    get_config_path(&filepath[0]);

    FILE *fp;
    fp = fopen(filepath, "r");
    if(fp) {
        size_t elems_read = fread(&flash_storage_buffer, 1, SETTINGS_SIZE, fp);
        if(elems_read < SETTINGS_SIZE) {
            fprintf(stderr, "Error reading config, less data than expected(%s): Read reports: %d: %s\n", filepath, errno, strerror(errno));
            memset(flash_storage_buffer, 0, sizeof(flash_storage_buffer)); // Zero results on error
            return NULL;
        }
        else if(ferror(fp) != 0) {
            fprintf(stderr, "Unable to read config(%s): %d: %s\n", filepath, errno, strerror(errno));
            memset(flash_storage_buffer, 0, sizeof(flash_storage_buffer));
            return NULL;
        }
        fclose(fp);
    }
    else {
        fprintf(stderr, "Unable to open config for reading(%s): %d: %s\n", filepath, errno, strerror(errno));
        memset(flash_storage_buffer, 0, sizeof(flash_storage_buffer));
        return NULL;
    }
    return &flash_storage_buffer[0];
}

// Write flash contents to filesystem
void write_flash_settings(uint8_t *buffer, size_t size) {    
    char filepath[PATH_MAX] = {0};
    get_config_path(&filepath[0]);

    FILE *fp;
    fp = fopen(filepath, "w");
    if(fp) {
        if(! (fwrite(buffer, size, 1, fp) == 1)) { // Returns num of writes successful
            fprintf(stderr, "Error while writing config(%s): %d: %s\n", filepath, errno, strerror(errno));
        }
        fclose(fp);
    }
    else {
        fprintf(stderr, "Unable to open config for writing(%s): %d: %s\n", filepath, errno, strerror(errno));
    }
}
