#ifndef MONOME_TEST_FAKE_PLATFORM_H
#define MONOME_TEST_FAKE_PLATFORM_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    FAKE_MONOME_SUCCESS,
    FAKE_MONOME_PARTIAL_THEN_RETRY,
    FAKE_MONOME_TIMEOUT,
    FAKE_MONOME_OPEN_FAILURE,
    FAKE_MONOME_WRITE_FAILURE,
    FAKE_MONOME_READ_FAILURE
} fake_monome_mode_t;

void fake_monome_reset(fake_monome_mode_t mode);
unsigned int fake_monome_open_count(void);
unsigned int fake_monome_close_count(void);
unsigned int fake_monome_query_write_count(unsigned int command);
size_t fake_monome_outstanding_allocations(void);
uint64_t fake_monome_elapsed_milliseconds(void);

#endif
