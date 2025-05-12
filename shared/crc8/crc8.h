/* Library modified and stripped down for use with Pico / Anachro Mouse
 *
 * See https://github.com/hdtodd/CRC8-Library for the original
 * 
 * My thanks to HD Todd for making their work available! 
 * - Aviancer <oss+amouse@skyvian.me>
*/

#ifndef CRC_H_
#define CRC_H_

#ifdef __linux__
#include <stdint.h>
#endif 

uint8_t crc8(uint8_t *msg, int sizeOfMsg, uint8_t init);

#endif // CRC_H_
