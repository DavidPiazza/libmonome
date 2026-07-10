#include "fake_platform.h"

#include <monomepp/Monome.hpp>

#include <iostream>
#include <stdexcept>
#include <utility>

namespace {

int failures = 0;

#define CHECK(expression)                                                        \
    do {                                                                         \
        if (!(expression)) {                                                     \
            std::cerr << __FILE__ << ':' << __LINE__ << ": CHECK failed: "    \
                      << #expression << '\n';                                    \
            ++failures;                                                          \
        }                                                                        \
    } while (false)

constexpr const char* device = "/dev/cu.usbmodemm07407811";

void testCallbacksSurviveMovesWithoutRebinding() {
    fake_monome_reset(FAKE_MONOME_SUCCESS);
    int calls = 0;
    monomepp::Monome first(device);
    first.on(monomepp::EventType::EncoderDelta,
             [&](const monomepp::Event& event) {
                 ++calls;
                 CHECK(event.encoderNumber() == 2);
                 CHECK(event.encoderDelta() == -3);
             });

    monomepp::Monome moved(std::move(first));
    CHECK(first.raw() == nullptr);
    fake_monome_dispatch(moved.raw(), MONOME_ENCODER_DELTA, 2, -3);
    CHECK(calls == 1);

    monomepp::Monome target(device);
    target = std::move(moved);
    CHECK(moved.raw() == nullptr);
    fake_monome_dispatch(target.raw(), MONOME_ENCODER_DELTA, 2, -3);
    CHECK(calls == 2);
}

void testRegistrationFailureDoesNotCommitCallbackState() {
    fake_monome_reset(FAKE_MONOME_SUCCESS);
    monomepp::Monome monome(device);
    int originalCalls = 0;
    int replacementCalls = 0;
    monome.on(monomepp::EventType::EncoderDelta,
              [&](const monomepp::Event&) { ++originalCalls; });

    fake_monome_fail_next_registration();
    bool caught = false;
    try {
        monome.on(monomepp::EventType::EncoderDelta,
                  [&](const monomepp::Event&) { ++replacementCalls; });
    } catch (const std::runtime_error&) {
        caught = true;
    }
    CHECK(caught);
    fake_monome_dispatch(monome.raw(), MONOME_ENCODER_DELTA, 0, 1);
    CHECK(originalCalls == 1);
    CHECK(replacementCalls == 0);

    fake_monome_fail_next_registration();
    caught = false;
    try {
        monome.on(monomepp::EventType::EncoderDelta, {});
    } catch (const std::runtime_error&) {
        caught = true;
    }
    CHECK(caught);
    fake_monome_dispatch(monome.raw(), MONOME_ENCODER_DELTA, 0, 1);
    CHECK(originalCalls == 2);
}

} // namespace

int main() {
    testCallbacksSurviveMovesWithoutRebinding();
    testRegistrationFailureDoesNotCommitCallbackState();

    if (failures != 0) {
        std::cerr << failures << " monome wrapper test(s) failed\n";
        return 1;
    }
    std::cout << "monome wrapper tests passed\n";
    return 0;
}
