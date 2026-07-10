#include "fake_platform.h"

#include <monomepp/Monome.hpp>

#include <array>
#include <cerrno>
#include <iostream>
#include <stdexcept>
#include <type_traits>
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

void testTypedOpenAndClosedHandleErrors() {
	static_assert(std::is_nothrow_destructible<monomepp::Monome>::value,
	              "Monome cleanup must be noexcept");
	static_assert(noexcept(std::declval<monomepp::Monome&>().eventNext(
	                  std::declval<monomepp::Event&>())),
	              "event polling must be noexcept");
	static_assert(noexcept(std::declval<monomepp::Monome&>().ledRingAll(0, 0)),
	              "ring output must be noexcept");

	fake_monome_reset(FAKE_MONOME_OPEN_FAILURE);
	bool caught = false;
	try {
		monomepp::Monome monome(device);
	} catch( const monomepp::Error& error ) {
		caught = true;
		CHECK(error.operation() == monomepp::Operation::Open);
		CHECK(error.nativeStatus() == ENODEV);
	}
	CHECK(caught);
	CHECK(fake_monome_open_count() == 1);
	CHECK(fake_monome_close_count() == 0);
	CHECK(fake_monome_outstanding_allocations() == 0);

	fake_monome_reset(FAKE_MONOME_SUCCESS);
	{
		monomepp::Monome first(device);
		monomepp::Monome owner(std::move(first));

		caught = false;
		try {
			(void) first.serial();
		} catch( const monomepp::Error& error ) {
			caught = true;
			CHECK(error.operation() == monomepp::Operation::ReadSerial);
			CHECK(error.nativeStatus() == EBADF);
		}
		CHECK(caught);

		monomepp::Event event;
		const auto poll = first.eventNext(event);
		CHECK(poll.status == monomepp::PollStatus::DeviceError);
		CHECK(poll.nativeStatus == EBADF);

		const auto output = first.ledRingAll(0, 0);
		CHECK(output.status == monomepp::IoStatus::DeviceError);
		CHECK(output.operation == monomepp::Operation::LedRingAll);
		CHECK(output.nativeStatus == EBADF);
	}
	CHECK(fake_monome_close_count() == 1);
	CHECK(fake_monome_outstanding_allocations() == 0);
}

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
    } catch (const monomepp::Error& error) {
        caught = true;
		CHECK(error.operation() == monomepp::Operation::ConfigureCallback);
		CHECK(error.nativeStatus() == EBUSY);
    }
    CHECK(caught);
    fake_monome_dispatch(monome.raw(), MONOME_ENCODER_DELTA, 0, 1);
    CHECK(originalCalls == 1);
    CHECK(replacementCalls == 0);

    fake_monome_fail_next_registration();
    caught = false;
    try {
        monome.on(monomepp::EventType::EncoderDelta, {});
    } catch (const monomepp::Error& error) {
        caught = true;
		CHECK(error.operation() == monomepp::Operation::ConfigureCallback);
		CHECK(error.nativeStatus() == EBUSY);
    }
    CHECK(caught);
    fake_monome_dispatch(monome.raw(), MONOME_ENCODER_DELTA, 0, 1);
    CHECK(originalCalls == 2);
}

void testPollResultsRetainNativeStatus() {
	fake_monome_reset(FAKE_MONOME_SUCCESS);
	monomepp::Monome monome(device);
	monomepp::Event event;

	auto result = monome.eventNext(event);
	CHECK(result.status == monomepp::PollStatus::NoData);
	CHECK(result.nativeStatus == 0);

	fake_monome_queue_encoder_delta(2, -3);
	result = monome.eventNext(event);
	CHECK(result.status == monomepp::PollStatus::Event);
	CHECK(result.nativeStatus > 0);
	CHECK(event.type() == monomepp::EventType::EncoderDelta);
	CHECK(event.encoderNumber() == 2);
	CHECK(event.encoderDelta() == -3);

	fake_monome_fail_next_read(-37);
	result = monome.eventNext(event);
	CHECK(result.status == monomepp::PollStatus::DeviceError);
	CHECK(result.nativeStatus == -37);
}

void testRingOutputResultsRetainNativeStatus() {
	fake_monome_reset(FAKE_MONOME_SUCCESS);
	monomepp::Monome monome(device);
	std::array<std::uint8_t, 64> levels{};
	levels[0] = 15;

	auto result = monome.ledRingMap(0, levels);
	CHECK(result.status == monomepp::IoStatus::Success);
	CHECK(result.operation == monomepp::Operation::LedRingMap);
	CHECK(result.nativeStatus > 0);

	fake_monome_fail_next_write(-23);
	result = monome.ledRingMap(1, levels);
	CHECK(result.status == monomepp::IoStatus::DeviceError);
	CHECK(result.operation == monomepp::Operation::LedRingMap);
	CHECK(result.nativeStatus == -23);

	result = monome.ledRingMap(0, static_cast<const std::uint8_t*>(nullptr));
	CHECK(result.status == monomepp::IoStatus::DeviceError);
	CHECK(result.operation == monomepp::Operation::LedRingMap);
	CHECK(result.nativeStatus == EINVAL);
}

void testCallbackExceptionsAreContainedAndCounted() {
	fake_monome_reset(FAKE_MONOME_SUCCESS);
	monomepp::Monome monome(device);
	monome.on(monomepp::EventType::EncoderDelta,
	          [](const monomepp::Event&) { throw std::runtime_error("scripted"); });

	CHECK(monome.callbackFailureCount() == 0);
	fake_monome_dispatch(monome.raw(), MONOME_ENCODER_DELTA, 0, 1);
	CHECK(monome.callbackFailureCount() == 1);

	fake_monome_queue_encoder_delta(1, 2);
	const auto result = monome.eventHandleNext();
	CHECK(result.status == monomepp::PollStatus::Event);
	CHECK(monome.callbackFailureCount() == 2);

	monomepp::Monome moved(std::move(monome));
	CHECK(monome.callbackFailureCount() == 0);
	CHECK(moved.callbackFailureCount() == 2);
	fake_monome_dispatch(moved.raw(), MONOME_ENCODER_DELTA, 0, 1);
	CHECK(moved.callbackFailureCount() == 3);
}

} // namespace

int main() {
	testTypedOpenAndClosedHandleErrors();
    testCallbacksSurviveMovesWithoutRebinding();
    testRegistrationFailureDoesNotCommitCallbackState();
	testPollResultsRetainNativeStatus();
	testRingOutputResultsRetainNativeStatus();
	testCallbackExceptionsAreContainedAndCounted();

    if (failures != 0) {
        std::cerr << failures << " monome wrapper test(s) failed\n";
        return 1;
    }
    std::cout << "monome wrapper tests passed\n";
    return 0;
}
