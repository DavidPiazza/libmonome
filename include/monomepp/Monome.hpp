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
#include <string>

#include <monome.h>

namespace monomepp {

class Monome {
public:
	using Callback = std::function<void(const Event&)>;

	explicit Monome(const std::string& device);
	Monome(const std::string& device, const std::string& listenPort);
	~Monome();

	Monome(const Monome&) = delete;
	Monome& operator=(const Monome&) = delete;

	Monome(Monome&& other) noexcept;
	Monome& operator=(Monome&& other) noexcept;

	monome_t* raw() const noexcept;

	std::string serial() const;
	std::string devpath() const;
	std::string friendlyName() const;
	std::string proto() const;
	int rows() const;
	int cols() const;

	void setRotation(Rotation rotation);
	Rotation rotation() const;

	int ledSet(unsigned int x, unsigned int y, bool on);
	int ledOn(unsigned int x, unsigned int y);
	int ledOff(unsigned int x, unsigned int y);
	int ledAll(bool on);
	int ledMap(unsigned int xOff, unsigned int yOff,
	           const std::array<std::uint8_t, 8>& data);
	int ledMap(unsigned int xOff, unsigned int yOff,
	           const std::uint8_t* data);
	int ledCol(unsigned int x, unsigned int yOff, std::size_t count,
	           const std::uint8_t* colData);
	int ledRow(unsigned int xOff, unsigned int y, std::size_t count,
	           const std::uint8_t* rowData);
	int ledIntensity(unsigned int brightness);

	int ledLevelSet(unsigned int x, unsigned int y, unsigned int level);
	int ledLevelAll(unsigned int level);
	int ledLevelMap(unsigned int xOff, unsigned int yOff,
	                const std::array<std::uint8_t, 64>& data);
	int ledLevelMap(unsigned int xOff, unsigned int yOff,
	                const std::uint8_t* data);
	int ledLevelRow(unsigned int xOff, unsigned int y, std::size_t count,
	                const std::uint8_t* data);
	int ledLevelCol(unsigned int x, unsigned int yOff, std::size_t count,
	                const std::uint8_t* data);

	int ledRingSet(unsigned int ring, unsigned int led, unsigned int level);
	int ledRingAll(unsigned int ring, unsigned int level);
	int ledRingMap(unsigned int ring,
	               const std::array<std::uint8_t, 64>& levels);
	int ledRingMap(unsigned int ring, const std::uint8_t* levels);
	int ledRingRange(unsigned int ring, unsigned int start, unsigned int end,
	                 unsigned int level);
	int ledRingIntensity(unsigned int brightness);

	int tiltEnable(unsigned int sensor);
	int tiltDisable(unsigned int sensor);

	int eventNext(Event& out);
	int eventHandleNext();
	void eventLoop();

	/*
	 * Callbacks live inside this object and are unregistered before close.
	 * The C API has a blocking event loop with no cancellation hook, so callers
	 * must not destroy or move a Monome while another thread is inside
	 * eventLoop() or monome_event_loop(raw()).
	 */
	void on(EventType type, Callback callback);

private:
	static constexpr std::size_t EventCount = MONOME_EVENT_MAX;

	monome_t* handle() const;
	void close();
	void unregisterCallbacks() noexcept;
	void rebindCallbacks() noexcept;
	void dispatch(const monome_event_t& event);

	static void handleEvent(const monome_event_t* event, void* data) noexcept;

	monome_t* monome_ = nullptr;
	std::array<Callback, EventCount> callbacks_{};
};

} // namespace monomepp

#endif // MONOMEPP_MONOME_HPP
