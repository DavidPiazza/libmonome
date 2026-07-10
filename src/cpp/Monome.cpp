/**
 * Copyright (c) 2026 David Piazza
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <monomepp/Monome.hpp>

#include <atomic>
#include <cerrno>
#include <sstream>
#include <utility>

namespace monomepp {
namespace {

constexpr const char* DefaultListenPort = "8000";

std::string stringFromNullable(const char* value) {
	return value ? std::string(value) : std::string();
}

bool eventIndex(monome_event_type_t type, std::size_t& index) noexcept {
	const auto value = static_cast<int>(type);
	if( value < 0 || value >= static_cast<int>(MONOME_EVENT_MAX) )
		return false;

	index = static_cast<std::size_t>(value);
	return true;
}

std::size_t eventIndex(EventType type, Operation operation) {
	std::size_t index = 0;
	if( !eventIndex(toC(type), index) )
		throw Error(operation, EINVAL, "unsupported monome event type");

	return index;
}

std::string makeErrorMessage(Operation operation, int nativeStatus,
							 const std::string& details) {
	std::ostringstream message;
	message << operationName(operation) << " failed (monome status "
	        << nativeStatus << ')';
	if( !details.empty() )
		message << ": " << details;
	return message.str();
}

IoResult makeIoResult(Operation operation, int nativeStatus) noexcept {
	return {
		nativeStatus < 0 ? IoStatus::DeviceError : IoStatus::Success,
		operation,
		nativeStatus
	};
}

IoResult closedIoResult(Operation operation) noexcept {
	return {IoStatus::DeviceError, operation, EBADF};
}

} // namespace

const char* operationName(Operation operation) noexcept {
	switch( operation ) {
	case Operation::None: return "none";
	case Operation::Open: return "open monome device";
	case Operation::ReadSerial: return "read monome serial";
	case Operation::ReadDevicePath: return "read monome device path";
	case Operation::ReadFriendlyName: return "read monome friendly name";
	case Operation::ReadProtocol: return "read monome protocol";
	case Operation::ReadRows: return "read monome rows";
	case Operation::ReadColumns: return "read monome columns";
	case Operation::SetRotation: return "set monome rotation";
	case Operation::ReadRotation: return "read monome rotation";
	case Operation::LedSet: return "set monome LED";
	case Operation::LedAll: return "set all monome LEDs";
	case Operation::LedMap: return "write monome LED map";
	case Operation::LedColumn: return "write monome LED column";
	case Operation::LedRow: return "write monome LED row";
	case Operation::LedIntensity: return "set monome LED intensity";
	case Operation::LedLevelSet: return "set monome LED level";
	case Operation::LedLevelAll: return "set all monome LED levels";
	case Operation::LedLevelMap: return "write monome LED level map";
	case Operation::LedLevelRow: return "write monome LED level row";
	case Operation::LedLevelColumn: return "write monome LED level column";
	case Operation::LedRingSet: return "set monome ring LED";
	case Operation::LedRingAll: return "set all monome ring LEDs";
	case Operation::LedRingMap: return "write monome ring LED map";
	case Operation::LedRingRange: return "write monome ring LED range";
	case Operation::LedRingIntensity: return "set monome ring intensity";
	case Operation::TiltEnable: return "enable monome tilt";
	case Operation::TiltDisable: return "disable monome tilt";
	case Operation::PollEvent: return "poll monome event";
	case Operation::EventLoop: return "run monome event loop";
	case Operation::ConfigureCallback: return "configure monome callback";
	}
	return "unknown monome operation";
}

Error::Error(Operation operation, int nativeStatus, std::string details)
	: std::runtime_error(makeErrorMessage(operation, nativeStatus, details)),
	  operation_(operation),
	  nativeStatus_(nativeStatus) {
}

Operation Error::operation() const noexcept {
	return operation_;
}

int Error::nativeStatus() const noexcept {
	return nativeStatus_;
}

struct Monome::CallbackState {
	std::array<Callback, EventCount> callbacks{};
	std::atomic<std::uint64_t> failureCount{0};
};

Monome::Monome(const std::string& device)
	: Monome(device, DefaultListenPort) {
}

Monome::Monome(const std::string& device, const std::string& listenPort)
	: callbackState_(std::make_unique<CallbackState>()) {
	errno = 0;
	monome_ = monome_open(device.c_str(),
	                      const_cast<char*>(listenPort.c_str()));
	if( !monome_ ) {
		const int nativeStatus = errno != 0 ? errno : -1;
		throw Error(Operation::Open, nativeStatus, device);
	}
}

Monome::~Monome() noexcept {
	close();
}

Monome::Monome(Monome&& other) noexcept
	: callbackState_(std::move(other.callbackState_)),
	  monome_(std::exchange(other.monome_, nullptr)) {
}

Monome& Monome::operator=(Monome&& other) noexcept {
	if( this == &other )
		return *this;

	close();

	callbackState_ = std::move(other.callbackState_);
	monome_ = std::exchange(other.monome_, nullptr);

	return *this;
}

monome_t* Monome::raw() const noexcept {
	return monome_;
}

std::string Monome::serial() const {
	return stringFromNullable(monome_get_serial(handle(Operation::ReadSerial)));
}

std::string Monome::devpath() const {
	return stringFromNullable(monome_get_devpath(handle(Operation::ReadDevicePath)));
}

std::string Monome::friendlyName() const {
	return stringFromNullable(
		monome_get_friendly_name(handle(Operation::ReadFriendlyName)));
}

std::string Monome::proto() const {
	return stringFromNullable(monome_get_proto(handle(Operation::ReadProtocol)));
}

int Monome::rows() const {
	return monome_get_rows(handle(Operation::ReadRows));
}

int Monome::cols() const {
	return monome_get_cols(handle(Operation::ReadColumns));
}

void Monome::setRotation(Rotation rotation) {
	monome_set_rotation(handle(Operation::SetRotation), toC(rotation));
}

Rotation Monome::rotation() const {
	return rotationFromC(monome_get_rotation(handle(Operation::ReadRotation)));
}

IoResult Monome::ledSet(unsigned int x, unsigned int y, bool on) noexcept {
	if( !monome_ )
		return closedIoResult(Operation::LedSet);
	return makeIoResult(Operation::LedSet,
	                    monome_led_set(monome_, x, y, on ? 1 : 0));
}

IoResult Monome::ledOn(unsigned int x, unsigned int y) noexcept {
	if( !monome_ )
		return closedIoResult(Operation::LedSet);
	return makeIoResult(Operation::LedSet, monome_led_on(monome_, x, y));
}

IoResult Monome::ledOff(unsigned int x, unsigned int y) noexcept {
	if( !monome_ )
		return closedIoResult(Operation::LedSet);
	return makeIoResult(Operation::LedSet, monome_led_off(monome_, x, y));
}

IoResult Monome::ledAll(bool on) noexcept {
	if( !monome_ )
		return closedIoResult(Operation::LedAll);
	return makeIoResult(Operation::LedAll, monome_led_all(monome_, on ? 1 : 0));
}

IoResult Monome::ledMap(unsigned int x, unsigned int y,
						const std::array<std::uint8_t, 8>& data) noexcept {
	return ledMap(x, y, data.data());
}

IoResult Monome::ledMap(unsigned int x, unsigned int y,
						const std::uint8_t* data) noexcept {
	if( !monome_ )
		return closedIoResult(Operation::LedMap);
	if( !data )
		return {IoStatus::DeviceError, Operation::LedMap, EINVAL};
	return makeIoResult(Operation::LedMap, monome_led_map(monome_, x, y, data));
}

IoResult Monome::ledCol(unsigned int x, unsigned int y, std::size_t count,
						const std::uint8_t* data) noexcept {
	if( !monome_ )
		return closedIoResult(Operation::LedColumn);
	if( count > 0 && !data )
		return {IoStatus::DeviceError, Operation::LedColumn, EINVAL};
	return makeIoResult(Operation::LedColumn,
	                    monome_led_col(monome_, x, y, count, data));
}

IoResult Monome::ledRow(unsigned int x, unsigned int y, std::size_t count,
						const std::uint8_t* data) noexcept {
	if( !monome_ )
		return closedIoResult(Operation::LedRow);
	if( count > 0 && !data )
		return {IoStatus::DeviceError, Operation::LedRow, EINVAL};
	return makeIoResult(Operation::LedRow,
	                    monome_led_row(monome_, x, y, count, data));
}

IoResult Monome::ledIntensity(unsigned int brightness) noexcept {
	if( !monome_ )
		return closedIoResult(Operation::LedIntensity);
	return makeIoResult(Operation::LedIntensity,
	                    monome_led_intensity(monome_, brightness));
}

IoResult Monome::ledLevelSet(unsigned int x, unsigned int y,
							 unsigned int level) noexcept {
	if( !monome_ )
		return closedIoResult(Operation::LedLevelSet);
	return makeIoResult(Operation::LedLevelSet,
	                    monome_led_level_set(monome_, x, y, level));
}

IoResult Monome::ledLevelAll(unsigned int level) noexcept {
	if( !monome_ )
		return closedIoResult(Operation::LedLevelAll);
	return makeIoResult(Operation::LedLevelAll,
	                    monome_led_level_all(monome_, level));
}

IoResult Monome::ledLevelMap(unsigned int x, unsigned int y,
							 const std::array<std::uint8_t, 64>& data) noexcept {
	return ledLevelMap(x, y, data.data());
}

IoResult Monome::ledLevelMap(unsigned int x, unsigned int y,
							 const std::uint8_t* data) noexcept {
	if( !monome_ )
		return closedIoResult(Operation::LedLevelMap);
	if( !data )
		return {IoStatus::DeviceError, Operation::LedLevelMap, EINVAL};
	return makeIoResult(Operation::LedLevelMap,
	                    monome_led_level_map(monome_, x, y, data));
}

IoResult Monome::ledLevelRow(unsigned int x, unsigned int y, std::size_t count,
							 const std::uint8_t* data) noexcept {
	if( !monome_ )
		return closedIoResult(Operation::LedLevelRow);
	if( count > 0 && !data )
		return {IoStatus::DeviceError, Operation::LedLevelRow, EINVAL};
	return makeIoResult(Operation::LedLevelRow,
	                    monome_led_level_row(monome_, x, y, count, data));
}

IoResult Monome::ledLevelCol(unsigned int x, unsigned int y, std::size_t count,
							 const std::uint8_t* data) noexcept {
	if( !monome_ )
		return closedIoResult(Operation::LedLevelColumn);
	if( count > 0 && !data )
		return {IoStatus::DeviceError, Operation::LedLevelColumn, EINVAL};
	return makeIoResult(Operation::LedLevelColumn,
	                    monome_led_level_col(monome_, x, y, count, data));
}

IoResult Monome::ledRingSet(unsigned int ring, unsigned int led,
							unsigned int level) noexcept {
	if( !monome_ )
		return closedIoResult(Operation::LedRingSet);
	return makeIoResult(Operation::LedRingSet,
	                    monome_led_ring_set(monome_, ring, led, level));
}

IoResult Monome::ledRingAll(unsigned int ring, unsigned int level) noexcept {
	if( !monome_ )
		return closedIoResult(Operation::LedRingAll);
	return makeIoResult(Operation::LedRingAll,
	                    monome_led_ring_all(monome_, ring, level));
}

IoResult Monome::ledRingMap(
		unsigned int ring,
		const std::array<std::uint8_t, 64>& levels) noexcept {
	return ledRingMap(ring, levels.data());
}

IoResult Monome::ledRingMap(unsigned int ring,
							const std::uint8_t* levels) noexcept {
	if( !monome_ )
		return closedIoResult(Operation::LedRingMap);
	if( !levels )
		return {IoStatus::DeviceError, Operation::LedRingMap, EINVAL};
	return makeIoResult(Operation::LedRingMap,
	                    monome_led_ring_map(monome_, ring, levels));
}

IoResult Monome::ledRingRange(unsigned int ring, unsigned int start,
							  unsigned int end, unsigned int level) noexcept {
	if( !monome_ )
		return closedIoResult(Operation::LedRingRange);
	return makeIoResult(Operation::LedRingRange,
	                    monome_led_ring_range(monome_, ring, start, end, level));
}

IoResult Monome::ledRingIntensity(unsigned int brightness) noexcept {
	if( !monome_ )
		return closedIoResult(Operation::LedRingIntensity);
	return makeIoResult(Operation::LedRingIntensity,
	                    monome_led_ring_intensity(monome_, brightness));
}

IoResult Monome::tiltEnable(unsigned int sensor) noexcept {
	if( !monome_ )
		return closedIoResult(Operation::TiltEnable);
	return makeIoResult(Operation::TiltEnable, monome_tilt_enable(monome_, sensor));
}

IoResult Monome::tiltDisable(unsigned int sensor) noexcept {
	if( !monome_ )
		return closedIoResult(Operation::TiltDisable);
	return makeIoResult(Operation::TiltDisable, monome_tilt_disable(monome_, sensor));
}

PollResult Monome::eventNext(Event& out) noexcept {
	if( !monome_ )
		return {PollStatus::DeviceError, EBADF};

	monome_event_t event{};
	const int nativeStatus = monome_event_next(monome_, &event);
	if( nativeStatus > 0 ) {
		out = Event(event);
		return {PollStatus::Event, nativeStatus};
	}
	if( nativeStatus == 0 )
		return {PollStatus::NoData, nativeStatus};
	return {PollStatus::DeviceError, nativeStatus};
}

PollResult Monome::eventHandleNext() noexcept {
	Event event;
	const auto result = eventNext(event);
	if( result.status != PollStatus::Event )
		return result;

	std::size_t index = 0;
	if( !callbackState_ || !eventIndex(toC(event.type()), index) ||
	    !callbackState_->callbacks[index] )
		return result;

	try {
		callbackState_->callbacks[index](event);
	} catch( ... ) {
		callbackState_->failureCount.fetch_add(1, std::memory_order_relaxed);
	}
	return result;
}

void Monome::eventLoop() {
	monome_event_loop(handle(Operation::EventLoop));
}

void Monome::on(EventType type, Callback callback) {
	auto* monome = handle(Operation::ConfigureCallback);
	const auto index = eventIndex(type, Operation::ConfigureCallback);
	const auto cType = toC(type);
	const int status = callback
		? monome_register_handler(monome, cType, &Monome::handleEvent,
		                          callbackState_.get())
		: monome_unregister_handler(monome, cType);

	if( status )
		throw Error(Operation::ConfigureCallback, status);

	// Commit only after the C registration operation succeeds. std::function's
	// move assignment is noexcept, so C and C++ callback state cannot diverge.
	callbackState_->callbacks[index] = std::move(callback);
}

std::uint64_t Monome::callbackFailureCount() const noexcept {
	return callbackState_
		? callbackState_->failureCount.load(std::memory_order_relaxed)
		: 0;
}

monome_t* Monome::handle(Operation operation) const {
	if( !monome_ )
		throw Error(operation, EBADF, "monome handle is not open");

	return monome_;
}

void Monome::close() noexcept {
	if( !monome_ )
		return;

	unregisterCallbacks();
	monome_close(monome_);
	monome_ = nullptr;
}

void Monome::unregisterCallbacks() noexcept {
	for( std::size_t i = 0; i < EventCount; ++i )
		monome_unregister_handler(monome_, static_cast<monome_event_type_t>(i));
}

void Monome::handleEvent(const monome_event_t* event, void* data) noexcept {
	try {
		if( !event || !data )
			return;

		auto& state = *static_cast<CallbackState*>(data);
		std::size_t index = 0;
		if( !eventIndex(event->event_type, index) || !state.callbacks[index] )
			return;

		Event wrapped(*event);
		state.callbacks[index](wrapped);
	} catch( ... ) {
		if( data ) {
			auto& state = *static_cast<CallbackState*>(data);
			state.failureCount.fetch_add(1, std::memory_order_relaxed);
		}
	}
}

} // namespace monomepp
