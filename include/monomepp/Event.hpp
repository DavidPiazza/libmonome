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

#ifndef MONOMEPP_EVENT_HPP
#define MONOMEPP_EVENT_HPP

#include "Types.hpp"

#include <monome.h>

namespace monomepp {

class Event {
public:
	Event() = default;
	explicit Event(const monome_event_t& event) noexcept;

	EventType type() const noexcept;

	unsigned int gridX() const noexcept;
	unsigned int gridY() const noexcept;
	unsigned int x() const noexcept;
	unsigned int y() const noexcept;

	unsigned int encoderNumber() const noexcept;
	int encoderDelta() const noexcept;

	unsigned int tiltSensor() const noexcept;
	int tiltX() const noexcept;
	int tiltY() const noexcept;
	int tiltZ() const noexcept;

	monome_t* monome() const noexcept;
	const monome_event_t& raw() const noexcept;

private:
	monome_event_t event_{};
};

inline Event::Event(const monome_event_t& event) noexcept
	: event_(event) {
}

inline EventType Event::type() const noexcept {
	return eventTypeFromC(event_.event_type);
}

inline unsigned int Event::gridX() const noexcept {
	return event_.grid.x;
}

inline unsigned int Event::gridY() const noexcept {
	return event_.grid.y;
}

inline unsigned int Event::x() const noexcept {
	return gridX();
}

inline unsigned int Event::y() const noexcept {
	return gridY();
}

inline unsigned int Event::encoderNumber() const noexcept {
	return event_.encoder.number;
}

inline int Event::encoderDelta() const noexcept {
	return event_.encoder.delta;
}

inline unsigned int Event::tiltSensor() const noexcept {
	return event_.tilt.sensor;
}

inline int Event::tiltX() const noexcept {
	return event_.tilt.x;
}

inline int Event::tiltY() const noexcept {
	return event_.tilt.y;
}

inline int Event::tiltZ() const noexcept {
	return event_.tilt.z;
}

inline monome_t* Event::monome() const noexcept {
	return event_.monome;
}

inline const monome_event_t& Event::raw() const noexcept {
	return event_;
}

} // namespace monomepp

#endif // MONOMEPP_EVENT_HPP
