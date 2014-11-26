/*
 * rb.cpp
 *
 *  Created on: 15 Oct 2014
 *      Author: michael.erskine
 */

#include "rb.h"
#include <stdint.h>
#include <string.h>

/**
 *	A happy little ring-buffer for foreground use (no concurrency support is
 *	provided _BY DESIGN_!!!)
 *
 *	Designed for simplicity and ease of understanding.
 *
 *	Designed to be unit tested to death.
 *
 *	No pointers, just array indices.
 *
 *	I had written an implementation of a non-overwriting ring-buffer that went
 *	to great lengths to avoid the head pointer meeting the tail pointer and
 *	having the buffer capacity being the array size -1. Everything was very
 *	efficient and deterministic and extra CPU cycles were used only when the
 *	buffer was approaching full.
 *
 *	All was well until trying to use it when I had potentially multiple items
 *	to add that depended on the amount of space available!
 *
 *	The complexity of having to calculate the amount of space available was not
 *	beyond me but it did require a diagram of the three cases to hold it in my
 *	mind: T==H, T<H, T>H.
 *
 *	Instead I decided to allow the head index meet the tail index and avoid
 *	having to deal with overlap by trusting a running space counter.
 *
 * TODO: should be a C++ class and have instances but meh, later!
 *
 */

typedef struct box_t {
	uint8_t buf[RINGBUFSIZE];
	uint16_t h;
	uint16_t t;
	uint16_t space;
} rb_t;

static rb_t rb;

void rb_clear(void) {
	memset((void *) (&rb), 0, sizeof(rb));
	rb.space = RINGBUFSIZE;
}

/**
 * Check for available space before calling: silently ignores request if no space!
 */
void rb_add(uint8_t b) {
	if (rb.space == 0)
		return;
	rb.buf[rb.h] = b;
	rb.h = (rb.h + 1) % RINGBUFSIZE;
	// this is safe because space must be > 0
	rb.space--;
}

/**
 * Check for available bytes before calling: returns zero if no bytes!
 */
uint8_t rb_take(void) {
	if (rb.space >= RINGBUFSIZE)
		return 0;
	uint8_t b = rb.buf[rb.t];
	rb.t = (rb.t + 1) % RINGBUFSIZE;
	// this is safe because space must be < RINGBUFSIZE
	rb.space++;
	return b;
}

uint16_t rb_bytes_available(void) {
	return RINGBUFSIZE - rb.space;
}

uint16_t rb_space_available(void) {
	return rb.space;
}

uint16_t rb_internal_get_buffer(uint8_t **pb) {
	*pb = rb.buf;
	return RINGBUFSIZE;
}

uint8_t rb_internal_get_head(void) {
	return rb.h;
}

uint8_t rb_internal_get_tail(void) {
	return rb.t;
}

uint8_t rb_internal_get_space(void) {
	return rb.space;
}
