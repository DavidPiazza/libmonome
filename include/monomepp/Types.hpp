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

#ifndef MONOMEPP_TYPES_HPP
#define MONOMEPP_TYPES_HPP

#include <monome.h>

namespace monomepp {

enum class EventType {
	ButtonUp = MONOME_BUTTON_UP,
	ButtonDown = MONOME_BUTTON_DOWN,
	EncoderDelta = MONOME_ENCODER_DELTA,
	EncoderKeyUp = MONOME_ENCODER_KEY_UP,
	EncoderKeyDown = MONOME_ENCODER_KEY_DOWN,
	Tilt = MONOME_TILT
};

enum class Rotation {
	Rotate0 = MONOME_ROTATE_0,
	Rotate90 = MONOME_ROTATE_90,
	Rotate180 = MONOME_ROTATE_180,
	Rotate270 = MONOME_ROTATE_270
};

inline monome_event_type_t toC(EventType type) noexcept {
	return static_cast<monome_event_type_t>(static_cast<int>(type));
}

inline EventType eventTypeFromC(monome_event_type_t type) noexcept {
	return static_cast<EventType>(static_cast<int>(type));
}

inline monome_rotate_t toC(Rotation rotation) noexcept {
	return static_cast<monome_rotate_t>(static_cast<int>(rotation));
}

inline Rotation rotationFromC(monome_rotate_t rotation) noexcept {
	return static_cast<Rotation>(static_cast<int>(rotation));
}

} // namespace monomepp

#endif // MONOMEPP_TYPES_HPP
