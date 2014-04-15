/* 
 * Copyright (c) 2014 Scott Vokes <vokes.s@gmail.com>
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 * 
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 * 
 * 3. This notice may not be removed or altered from any source
 * distribution.
 */

#ifndef BLUEBOTTLE_H
#define BLUEBOTTLE_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/* Bit-bang async serial IO (UARTs). Currently assumes LSB, 8N1. */

#define BLUEBOTTLE_VERSION_MAJOR 0
#define BLUEBOTTLE_VERSION_MINOR 1
#define BLUEBOTTLE_VERSION_PATCH 0

/* Callback for when serial data has been successfully read.
 * UDATA is the pointer provided during bluebottle_init/new, or NULL. */
typedef void bluebottle_read_cb(uint8_t *buf, size_t size, void *udata);

typedef enum {
    MODE_DONE,                  /* not currently writing */
    MODE_START_BIT,             /* on start bit */
    MODE_BITS,                  /* on payload bits */
    MODE_STOP_BIT,              /* on stop bit */
} buf_mode;

typedef struct {
    uint8_t *buf;
    size_t size;
    size_t index;
    buf_mode mode;
    uint8_t bit_i;
} buffer;

typedef struct bluebottle {
    buffer in;
    buffer out;
    size_t out_size;
    bluebottle_read_cb *read_cb;
    void *read_udata;
} bluebottle;

/* Configuration for a bit-banged serial handle.
 * IN_BUF and OUT_BUF are optional (for connections that are
 * TX or RX only), in which case the sizes must be NULL.
 *
 * READ_UDATA is an argument that wil be passed along to
 * the optional READ_CB for if data buffer is received. */
struct bluebottle_cfg {
    uint8_t *in_buf;
    size_t in_buf_size;
    uint8_t *out_buf;
    size_t out_buf_size;
    bluebottle_read_cb *read_cb; /* callback for successful read */
    void *read_udata;            /* closure for successful read */
};

/* Initialize a bluebottle handle according to the config.
 * Returns true on success, false on error. */
bool bluebottle_init(bluebottle *s, struct bluebottle_cfg *config);

/* Allocate and initialize a bluebottle handle according to the config.
 * Returns NULL on error. */
bluebottle *bluebottle_new(struct bluebottle_cfg *config);

typedef enum {
    BLUEBOTTLE_WRITE_ENQUEUE_OK,
    BLUEBOTTLE_WRITE_ENQUEUE_ERROR_NULL = -1,
    BLUEBOTTLE_WRITE_ENQUEUE_ERROR_TOO_LARGE = -2,
    BLUEBOTTLE_WRITE_ENQUEUE_ERROR_BUSY = -3,
} bluebottle_write_enqueue_res;

/* Enqueue an outgoing message, to be clocked out by
 * bluebottle_write_step. */
bluebottle_write_enqueue_res bluebottle_write_enqueue(bluebottle *s,
    uint8_t *msg, size_t size);

/* Is it currently busy with a write? */
bool bluebottle_write_busy(bluebottle *s);

/* Abort the current write.
 * Returns false on error (not writing or s == NULL). */
bool bluebottle_write_abort(bluebottle *s);

typedef enum {
    BLUEBOTTLE_WRITE_STEP_LOW,  /* logic low */
    BLUEBOTTLE_WRITE_STEP_HIGH, /* logic high */
    BLUEBOTTLE_WRITE_STEP_ERROR_NULL = -1,
} bluebottle_write_step_res;

/* Step the current write, if any, and indicate whether the
 * TX line should be logic low or high.
 * 
 * Must be called at a regular interval, according to the known
 * baud rate. */
bluebottle_write_step_res bluebottle_write_step(bluebottle *s);

typedef enum {
    BLUEBOTTLE_READ_SINK_OK,
    BLUEBOTTLE_READ_SINK_GOT_MESSAGE,
    BLUEBOTTLE_READ_SINK_ERROR_NULL = -1,
    BLUEBOTTLE_READ_SINK_ERROR_BUFFER_FULL = -2,
    BLUEBOTTLE_READ_SINK_ERROR_BAD_MESSAGE = -3,
} bluebottle_read_sink_res;

/* Sink a bit read from the RX line. If the transfer has completed,
 * a callback (if registered) will be called with the payload.
 * 
 * Must be called at a regular interval, according to the known baud rate.*/
bluebottle_read_sink_res bluebottle_read_sink(bluebottle *s, bool bit);

/* Free the bluebottle struct.
 * Note: Does NOT free the buffer(s) passed to it on init. */
void bluebottle_free(bluebottle *s);

#endif
