/* 
 * Copyright (c) 2014 Scott Vokes
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

#include <string.h>

#include "bluebottle.h"

bluebottle *bluebottle_new(struct bluebottle_cfg *config) {
    bluebottle *s = NULL;
    if (config == NULL) { goto fail; }
    s = malloc(sizeof(*s));
    if (s == NULL) { goto fail; }

    if (!bluebottle_init(s, config)) { goto fail; }
    return s;

fail:
    if (s) { free(s); }
    return NULL;
}

bool bluebottle_init(bluebottle *s, struct bluebottle_cfg *config) {
    if ((config->in_buf == NULL && config->in_buf_size != 0)
        || (config->out_buf == NULL && config->out_buf_size != 0)) {
        return false;              /* API misuse */
    }

    memset(s, 0, sizeof(s));
    s->in.buf = config->in_buf;
    s->in.size = config->in_buf_size;
    s->in.bit_i = 0x01;           /* LSB */
    s->in.mode = MODE_DONE;
    s->out.buf = config->out_buf;
    s->out.size = config->out_buf_size;
    s->out.bit_i = 0x01;
    s->out.mode = MODE_DONE;
    s->read_cb = config->read_cb;
    s->read_udata = config->read_udata;
    return true;
}

bluebottle_write_enqueue_res bluebottle_write_enqueue(bluebottle *s,
    uint8_t *msg, size_t size) {
    if (s == NULL) { return BLUEBOTTLE_WRITE_ENQUEUE_ERROR_NULL; }
    if (bluebottle_write_busy(s)) { 
        return BLUEBOTTLE_WRITE_ENQUEUE_ERROR_BUSY;
    }

    if (s->out.size < size) {
        return BLUEBOTTLE_WRITE_ENQUEUE_ERROR_TOO_LARGE;
    }

    memcpy(s->out.buf, msg, size);
    s->out_size = size;
    s->out.index = 0;
    s->out.bit_i = 0x01;
    s->out.mode = MODE_START_BIT;

    return BLUEBOTTLE_WRITE_ENQUEUE_OK;
}

/* Is it currently busy with a write? */
bool bluebottle_write_busy(bluebottle *s) {
    return s && s->out.mode != MODE_DONE;
}

/* Abort the current write.
 * Returns false on error (not writing or s == NULL). */
bool bluebottle_write_abort(bluebottle *s) {
    if (s == NULL) { return false; }
    s->out.mode = MODE_DONE;
    return true;
}

/* Step the current write, if any, and indicate whether the
 * TX line should be logic low, high, or left high after completion.
 * Must be called at a regular interval, according to the known
 * baud rate. */
bluebottle_write_step_res bluebottle_write_step(bluebottle *s) {
    if (s == NULL) { return BLUEBOTTLE_WRITE_STEP_ERROR_NULL; }

    switch (s->out.mode) {
    case MODE_DONE:
    default:
        return BLUEBOTTLE_WRITE_STEP_HIGH; /* idle: logic high */
    case MODE_START_BIT:
        s->out.mode = MODE_BITS;
        return BLUEBOTTLE_WRITE_STEP_LOW;  /* start bit: low */
        break;
    case MODE_BITS:
    {
        uint8_t bit = (s->out.buf[s->out.index] & s->out.bit_i);
        if (s->out.bit_i == 0x80) {
            s->out.mode = MODE_STOP_BIT;
            s->out.index++;
        } else {
            s->out.bit_i <<= 1;
        }
        return bit ? BLUEBOTTLE_WRITE_STEP_HIGH : BLUEBOTTLE_WRITE_STEP_LOW;
    }
    case MODE_STOP_BIT:
        if (s->out.index == s->out_size) {
            s->out.mode = MODE_DONE;
        } else {
            s->out.bit_i = 0x01;
            s->out.mode = MODE_START_BIT;
        }

        return BLUEBOTTLE_WRITE_STEP_HIGH;  /* stop bit: high */
    }
}

/* Sink a bit read from the RX line. If the transfer has completed,
 * a callback (if registered) will be called with the payload.
 * Must be called at a regular interval, according to the known baud rate.*/
bluebottle_read_sink_res bluebottle_read_sink(bluebottle *s, bool bit) {
    if (s == NULL) { return BLUEBOTTLE_READ_SINK_ERROR_NULL; }

    //printf("MODE %d, bit %d\n", s->in.mode, bit);
    switch (s->in.mode) {
    default:
    case MODE_DONE:
        if (bit) {                        /* still idle */
            return BLUEBOTTLE_READ_SINK_OK;
        }
        if (s->in.buf == NULL) {
            return BLUEBOTTLE_READ_SINK_ERROR_NULL;
        }
        s->in.index = 0;
        memset(s->in.buf, 0, s->in.size);
        /* fall through */
    case MODE_START_BIT:
        if (bit) {              /* idle/high: done */
            if (s->read_cb) {
                s->read_cb(s->in.buf, s->in.index, s->read_udata);
            }
            s->in.mode = MODE_DONE;
            return BLUEBOTTLE_READ_SINK_GOT_MESSAGE;
        } else {                /* start bit */
            s->in.mode = MODE_BITS;
            s->in.bit_i = 0x01;
            if (s->in.index == s->in.size) {
                s->in.mode = MODE_DONE;
                return BLUEBOTTLE_READ_SINK_ERROR_BUFFER_FULL;
            }
            
            return BLUEBOTTLE_READ_SINK_OK;
        }
    case MODE_BITS:
        if (bit) {
            s->in.buf[s->in.index] |= s->in.bit_i;
        }
        s->in.bit_i <<= 1;
        if (s->in.bit_i == 0x00) {
            s->in.mode = MODE_STOP_BIT;
        }
        return BLUEBOTTLE_READ_SINK_OK;
    case MODE_STOP_BIT:
        if (bit) {
            s->in.index++;
            s->in.mode = MODE_START_BIT;
            return BLUEBOTTLE_READ_SINK_OK;
        } else {                /* bad stop bit */
            s->in.mode = MODE_DONE;
            return BLUEBOTTLE_READ_SINK_ERROR_BAD_MESSAGE;
        }
    }
}

void bluebottle_free(bluebottle *s) { free(s); }
