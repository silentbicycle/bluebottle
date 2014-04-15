#include "greatest.h"
#include "bluebottle.h"

#if 0
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...)
#endif

TEST bluebottle_new_should_allocate(void) {
    struct bluebottle_cfg cfg = {
        .read_udata = NULL,
    };
    bluebottle *s = bluebottle_new(&cfg);
    ASSERT(s);
    bluebottle_free(s);
    PASS();
}

static uint8_t msg[] = "such bits, very UART, wow";

TEST enqueue_and_step_through_msg(void) {
    static uint8_t out_buf[64];

    struct bluebottle_cfg cfg = {
        .out_buf = out_buf,
        .out_buf_size = sizeof(out_buf),
    };
    bluebottle *s = bluebottle_new(&cfg);
    ASSERT(s);

    bluebottle_write_enqueue_res eres;
    eres = bluebottle_write_enqueue(s, msg, sizeof(msg));
    ASSERT_EQ(BLUEBOTTLE_WRITE_ENQUEUE_OK, eres);
    
    int bit_i = 0;

    LOG("\n");
    while (bluebottle_write_busy(s)) {
        bluebottle_write_step_res sres;
        sres = bluebottle_write_step(s);
        ASSERT(sres != BLUEBOTTLE_WRITE_STEP_ERROR_NULL);
        if (bit_i % 10 == 0 && bit_i != 0) { LOG("\n"); }
        LOG(sres == BLUEBOTTLE_WRITE_STEP_LOW ? " l" : " H");
        bit_i++;
    }
    if (bit_i % 10 != 0) { LOG("\n"); }

    bluebottle_free(s);
    PASS();
}

typedef struct {
    bool pass;
} test_udata;

static void got_msg_cb(uint8_t *buf, size_t size, void *udata) {
    test_udata *ud = (test_udata *)udata;
    size_t msg_sz = sizeof(msg);
    size_t min_sz = (size < msg_sz ? size : msg_sz);
    if (0 == memcmp(buf, msg, min_sz)) {
        ud->pass = true;
    }
}

TEST sink_msg_and_compare(void) {
    static uint8_t in_buf[64];

    test_udata ud;
    memset(&ud, 0, sizeof(ud));

    struct bluebottle_cfg cfg = {
        .in_buf = in_buf,
        .in_buf_size = sizeof(in_buf),
        .read_cb = got_msg_cb,
        .read_udata = (void *)&ud,
    };
    bluebottle *s = bluebottle_new(&cfg);
    ASSERT(s);

    bluebottle_read_sink_res sres;
    for (uint8_t byte_i = 0; byte_i < sizeof(msg); byte_i++) {
        sres = bluebottle_read_sink(s, 0); /* start bit */
        ASSERT_EQ(BLUEBOTTLE_READ_SINK_OK, sres);
        uint8_t byte = msg[byte_i];
        for (uint8_t bit_i = 0x01; bit_i; bit_i <<= 1) {
            sres = bluebottle_read_sink(s, byte & bit_i);
        ASSERT_EQ(BLUEBOTTLE_READ_SINK_OK, sres);
        }
        sres = bluebottle_read_sink(s, 1); /* stop bit */
        ASSERT_EQ(BLUEBOTTLE_READ_SINK_OK, sres);
    }
    sres = bluebottle_read_sink(s, 1);     /* extra stop bit -> done */
    ASSERT_EQ(BLUEBOTTLE_READ_SINK_GOT_MESSAGE, sres);
    ASSERT(ud.pass);
    bluebottle_free(s);
    PASS();
}

SUITE(suite) {
    RUN_TEST(bluebottle_new_should_allocate);
    RUN_TEST(enqueue_and_step_through_msg);
    RUN_TEST(sink_msg_and_compare);
}

/* Add all the definitions that need to be in the test runner's main file. */
GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();      /* command-line arguments, initialization. */
    RUN_SUITE(suite);
    GREATEST_MAIN_END();        /* display results */
}
