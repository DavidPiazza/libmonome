#include "fake_platform.h"

#include "platform.h"

#include <stdlib.h>
#include <string.h>

enum {
    system_get_grid_size = 0x5
};

typedef struct {
    fake_monome_mode_t mode;
    unsigned int opens;
    unsigned int closes;
    unsigned int writes[16];
    size_t allocations;
    uint64_t now_ms;
    uint8_t input[128];
    size_t input_size;
    size_t input_offset;
    int queued_initial;
    int queued_retry;
} fake_state_t;

static fake_state_t state;

static void append_byte(uint8_t value) {
    state.input[state.input_size++] = value;
}

static void append_initial_responses(int include_grid_size) {
    static const char identifier[32] = "m07407811";
    size_t index;

    append_byte(0x00);
    append_byte(0);
    append_byte(0);
    append_byte(0x01);
    for( index = 0; index < sizeof(identifier); ++index )
        append_byte((uint8_t) identifier[index]);
    if( include_grid_size ) {
        append_byte(0x03);
        append_byte(0);
        append_byte(0);
    }
}

void fake_monome_reset(fake_monome_mode_t mode) {
    memset(&state, 0, sizeof(state));
    state.mode = mode;
}

unsigned int fake_monome_open_count(void) { return state.opens; }
unsigned int fake_monome_close_count(void) { return state.closes; }
unsigned int fake_monome_query_write_count(unsigned int command) {
    return command < 16 ? state.writes[command] : 0;
}
size_t fake_monome_outstanding_allocations(void) { return state.allocations; }
uint64_t fake_monome_elapsed_milliseconds(void) { return state.now_ms; }

char *monome_platform_get_dev_serial(const char *device) {
    (void) device;
    return m_strdup("m07407811");
}

int monome_platform_open(monome_t *monome, const monome_devmap_t *mapping,
                         const char *device) {
    (void) mapping;
    (void) device;
    ++state.opens;
    if( state.mode == FAKE_MONOME_OPEN_FAILURE )
        return 1;
    monome->fd = 42;
    return 0;
}

int monome_platform_close(monome_t *monome) {
    if( monome->fd >= 0 ) {
        ++state.closes;
        monome->fd = -1;
    }
    return 0;
}

ssize_t monome_platform_write(monome_t *monome, const uint8_t *buffer,
                              size_t byte_count) {
    unsigned int command;
    (void) monome;
    if( state.mode == FAKE_MONOME_WRITE_FAILURE )
        return -1;
    if( byte_count == 0 )
        return 0;
    command = buffer[0] & 0x0f;
    ++state.writes[command];
    return (ssize_t) byte_count;
}

ssize_t monome_platform_read(monome_t *monome, uint8_t *buffer,
                             size_t byte_count) {
    size_t available;
    size_t copied;
    (void) monome;
    if( state.mode == FAKE_MONOME_READ_FAILURE )
        return -1;
    available = state.input_size - state.input_offset;
    if( available == 0 )
        return 0;
    copied = available < byte_count ? available : byte_count;
    memcpy(buffer, state.input + state.input_offset, copied);
    state.input_offset += copied;
    return (ssize_t) copied;
}

int monome_platform_wait_for_input(monome_t *monome, uint_t milliseconds) {
    (void) monome;
    state.now_ms += milliseconds;

    if( state.mode == FAKE_MONOME_READ_FAILURE )
        return 0;

    if( !state.queued_initial &&
        (state.mode == FAKE_MONOME_SUCCESS ||
         state.mode == FAKE_MONOME_PARTIAL_THEN_RETRY) ) {
        append_initial_responses(state.mode == FAKE_MONOME_SUCCESS);
        state.queued_initial = 1;
    }

    if( state.mode == FAKE_MONOME_PARTIAL_THEN_RETRY &&
        !state.queued_retry &&
        state.writes[system_get_grid_size] >= 2 ) {
        append_byte(0x03);
        append_byte(0);
        append_byte(0);
        state.queued_retry = 1;
    }

    return state.input_offset < state.input_size ? 0 : 1;
}

uint64_t monome_platform_monotonic_milliseconds(void) {
    return state.now_ms;
}

void monome_event_loop(monome_t *monome) { (void) monome; }

void *m_malloc(size_t size) {
    void *value = malloc(size);
    if( value )
        ++state.allocations;
    return value;
}

void *m_calloc(size_t count, size_t size) {
    void *value = calloc(count, size);
    if( value )
        ++state.allocations;
    return value;
}

void *m_strdup(const char *value) {
    size_t size = strlen(value) + 1;
    char *copy = m_malloc(size);
    if( copy )
        memcpy(copy, value, size);
    return copy;
}

void m_free(void *value) {
    if( value ) {
        --state.allocations;
        free(value);
    }
}

void m_sleep(uint_t milliseconds) { state.now_ms += milliseconds; }
