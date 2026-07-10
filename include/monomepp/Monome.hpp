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

#ifndef MONOMEPP_MONOME_HPP
#define MONOMEPP_MONOME_HPP

#include "Event.hpp"
#include "Types.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

#include <monome.h>

namespace monomepp {

enum class Operation {
	None,
	Open,
	ReadSerial,
	ReadDevicePath,
	ReadFriendlyName,
	ReadProtocol,
	ReadRows,
	ReadColumns,
	SetRotation,
	ReadRotation,
	LedSet,
	LedAll,
	LedMap,
	LedColumn,
	LedRow,
	LedIntensity,
	LedLevelSet,
	LedLevelAll,
	LedLevelMap,
	LedLevelRow,
	LedLevelColumn,
	LedRingSet,
	LedRingAll,
	LedRingMap,
	LedRingRange,
	LedRingIntensity,
	TiltEnable,
	TiltDisable,
	PollEvent,
	EventLoop,
	ConfigureCallback
};

const char* operationName(Operation operation) noexcept;

class Error : public std::runtime_error {
public:
	Error(Operation operation, int nativeStatus, std::string details = {});

	Operation operation() const noexcept;
	int nativeStatus() const noexcept;

private:
	Operation operation_ = Operation::None;
	int nativeStatus_ = 0;
};

enum class PollStatus {
	Event,
	NoData,
	DeviceError
};

// eventNext() changes its Event output only when status is Event. nativeStatus
// is the unmodified libmonome return value: positive for an event, zero for no
// data, and negative for a device error.
struct PollResult {
	PollStatus status = PollStatus::NoData;
	int nativeStatus = 0;
};

enum class IoStatus {
	Success,
	DeviceError
};

// libmonome output functions use both zero and positive byte counts for success.
// IoStatus normalizes that convention while nativeStatus remains inspectable.
struct IoResult {
	IoStatus status = IoStatus::Success;
	Operation operation = Operation::None;
	int nativeStatus = 0;
};

class Monome {
public:
	using Callback = std::function<void(const Event&)>;

	explicit Monome(const std::string& device);
	Monome(const std::string& device, const std::string& listenPort);
	~Monome() noexcept;

	Monome(const Monome&) = delete;
	Monome& operator=(const Monome&) = delete;

	Monome(Monome&& other) noexcept;
	Monome& operator=(Monome&& other) noexcept;

	/*
	 * Borrowed C handle. Monome retains ownership. It is invalidated by this
	 * object's destruction, move assignment, or a move that makes this object
	 * moved-from. Callers must never pass it to monome_close().
	 */
	monome_t* raw() const noexcept;

	std::string serial() const;
	std::string devpath() const;
	std::string friendlyName() const;
	std::string proto() const;
	int rows() const;
	int cols() const;

	void setRotation(Rotation rotation);
	Rotation rotation() const;

	IoResult ledSet(unsigned int x, unsigned int y, bool on) noexcept;
	IoResult ledOn(unsigned int x, unsigned int y) noexcept;
	IoResult ledOff(unsigned int x, unsigned int y) noexcept;
	IoResult ledAll(bool on) noexcept;
	IoResult ledMap(unsigned int x, unsigned int y,
	                const std::array<std::uint8_t, 8>& data) noexcept;
	IoResult ledMap(unsigned int x, unsigned int y,
	                const std::uint8_t* data) noexcept;
	IoResult ledCol(unsigned int x, unsigned int y, std::size_t count,
	                const std::uint8_t* data) noexcept;
	IoResult ledRow(unsigned int x, unsigned int y, std::size_t count,
	                const std::uint8_t* data) noexcept;
	IoResult ledIntensity(unsigned int brightness) noexcept;

	IoResult ledLevelSet(unsigned int x, unsigned int y,
	                     unsigned int level) noexcept;
	IoResult ledLevelAll(unsigned int level) noexcept;
	IoResult ledLevelMap(unsigned int x, unsigned int y,
	                     const std::array<std::uint8_t, 64>& data) noexcept;
	IoResult ledLevelMap(unsigned int x, unsigned int y,
	                     const std::uint8_t* data) noexcept;
	IoResult ledLevelRow(unsigned int x, unsigned int y, std::size_t count,
	                     const std::uint8_t* data) noexcept;
	IoResult ledLevelCol(unsigned int x, unsigned int y, std::size_t count,
	                     const std::uint8_t* data) noexcept;

	IoResult ledRingSet(unsigned int ring, unsigned int led,
	                    unsigned int level) noexcept;
	IoResult ledRingAll(unsigned int ring, unsigned int level) noexcept;
	IoResult ledRingMap(unsigned int ring,
	                    const std::array<std::uint8_t, 64>& levels) noexcept;
	IoResult ledRingMap(unsigned int ring, const std::uint8_t* levels) noexcept;
	IoResult ledRingRange(unsigned int ring, unsigned int start,
	                      unsigned int end, unsigned int level) noexcept;
	IoResult ledRingIntensity(unsigned int brightness) noexcept;

	IoResult tiltEnable(unsigned int sensor) noexcept;
	IoResult tiltDisable(unsigned int sensor) noexcept;

	// Polling is nonthrowing so device workers can treat absence and I/O loss as
	// status. A closed or moved-from Monome reports DeviceError with EBADF.
	PollResult eventNext(Event& out) noexcept;
	PollResult eventHandleNext() noexcept;
	void eventLoop();

	/*
	 * Callbacks live inside this object and are unregistered before close.
	 * Callback registration and dispatch share an unsynchronized callback table:
	 * callers must not invoke on() concurrently with eventHandleNext(),
	 * eventLoop(), or native callback dispatch.
	 * The C API has a blocking event loop with no cancellation hook, so callers
	 * must not destroy or move a Monome while another thread is inside
	 * eventLoop() or monome_event_loop(raw()).
	 */
	void on(EventType type, Callback callback);
	// Exceptions thrown by user callbacks are caught at both callback dispatch
	// boundaries and counted. The count follows ownership across moves.
	std::uint64_t callbackFailureCount() const noexcept;

private:
	static constexpr std::size_t EventCount = MONOME_EVENT_MAX;
	struct CallbackState;

	monome_t* handle(Operation operation) const;
	void close() noexcept;
	void unregisterCallbacks() noexcept;

	static void handleEvent(const monome_event_t* event, void* data) noexcept;

	// The C API stores this address as callback user data. Heap ownership keeps
	// it stable when Monome itself moves.
	std::unique_ptr<CallbackState> callbackState_;
	monome_t* monome_ = nullptr;
};

} // namespace monomepp

#endif // MONOMEPP_MONOME_HPP
