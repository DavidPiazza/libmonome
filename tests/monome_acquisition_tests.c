#include "fake_platform.h"

#include <monome.h>

#include <stdio.h>

static int failures;

#define CHECK(expression)                                                        \
    do {                                                                         \
        if( !(expression) ) {                                                    \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n",                       \
                    __FILE__, __LINE__, #expression);                            \
            ++failures;                                                          \
        }                                                                        \
    } while( 0 )

static monome_t *open_arc(void) {
    return monome_open("/dev/cu.usbmodemm07407811", "8000");
}

static void test_successful_handshake(void) {
    monome_t *monome;
    fake_monome_reset(FAKE_MONOME_SUCCESS);
    monome = open_arc();
    CHECK(monome != NULL);
    CHECK(fake_monome_open_count() == 1);
    CHECK(fake_monome_query_write_count(0x0) == 1);
    CHECK(fake_monome_query_write_count(0x1) == 1);
    CHECK(fake_monome_query_write_count(0x5) == 1);
    if( monome )
        monome_close(monome);
    CHECK(fake_monome_close_count() == 1);
    CHECK(fake_monome_outstanding_allocations() == 0);
}

static void test_partial_response_retries_only_missing_query(void) {
    monome_t *monome;
    fake_monome_reset(FAKE_MONOME_PARTIAL_THEN_RETRY);
    monome = open_arc();
    CHECK(monome != NULL);
    CHECK(fake_monome_query_write_count(0x0) == 1);
    CHECK(fake_monome_query_write_count(0x1) == 1);
    CHECK(fake_monome_query_write_count(0x5) == 2);
    CHECK(fake_monome_elapsed_milliseconds() >= 250);
    if( monome )
        monome_close(monome);
    CHECK(fake_monome_close_count() == 1);
    CHECK(fake_monome_outstanding_allocations() == 0);
}

static void test_timeout_is_bounded_and_cleans_up(void) {
    fake_monome_reset(FAKE_MONOME_TIMEOUT);
    CHECK(open_arc() == NULL);
    CHECK(fake_monome_elapsed_milliseconds() >= 1000);
    CHECK(fake_monome_elapsed_milliseconds() <= 1025);
    CHECK(fake_monome_close_count() == 1);
    CHECK(fake_monome_query_write_count(0x0) == 2);
    CHECK(fake_monome_query_write_count(0x1) == 2);
    CHECK(fake_monome_query_write_count(0x5) == 2);
    CHECK(fake_monome_outstanding_allocations() == 0);
}

static void test_failures_clean_up_exactly_once(void) {
    fake_monome_reset(FAKE_MONOME_OPEN_FAILURE);
    CHECK(open_arc() == NULL);
    CHECK(fake_monome_close_count() == 0);
    CHECK(fake_monome_outstanding_allocations() == 0);

    fake_monome_reset(FAKE_MONOME_WRITE_FAILURE);
    CHECK(open_arc() == NULL);
    CHECK(fake_monome_close_count() == 1);
    CHECK(fake_monome_outstanding_allocations() == 0);

    fake_monome_reset(FAKE_MONOME_READ_FAILURE);
    CHECK(open_arc() == NULL);
    CHECK(fake_monome_close_count() == 1);
    CHECK(fake_monome_outstanding_allocations() == 0);
}

int main(void) {
    test_successful_handshake();
    test_partial_response_retries_only_missing_query();
    test_timeout_is_bounded_and_cleans_up();
    test_failures_clean_up_exactly_once();

    if( failures ) {
        fprintf(stderr, "%d monome acquisition test(s) failed\n", failures);
        return 1;
    }
    puts("monome acquisition tests passed");
    return 0;
}
