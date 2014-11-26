/*
 * rb.h
 *
 *  Created on: 15 Oct 2014
 *      Author: michael.erskine
 */

#ifndef RB_H_
#define RB_H_

#include <stdint.h>

// Buffer size is configurable at compile time by defining RB_RINGBUFSIZE
// Naturally it needs to be a valid 16-bit positive integer of reasonable size!
#ifdef RB_RINGBUFSIZE
#define RINGBUFSIZE RB_RINGBUFSIZE
#else
#define RINGBUFSIZE 200
#endif


#ifdef __cplusplus
extern "C" {
#endif

void rb_clear(void);
void rb_add(uint8_t b);
uint8_t rb_take(void);
uint16_t rb_bytes_available(void);
uint16_t rb_space_available(void);

#ifdef __cplusplus
}
#endif


#endif /* RB_H_ */
