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

#include <exception>
#include <stdexcept>
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

std::size_t eventIndex(EventType type) {
	std::size_t index = 0;
	if( !eventIndex(toC(type), index) )
		throw std::invalid_argument("unsupported monome event type");

	return index;
}

} // namespace

Monome::Monome(const std::string& device)
	: Monome(device, DefaultListenPort) {
}

Monome::Monome(const std::string& device, const std::string& listenPort)
	: monome_(monome_open(device.c_str(),
	                      const_cast<char*>(listenPort.c_str()))) {
	if( !monome_ )
		throw std::runtime_error("failed to open monome device: " + device);
}

Monome::~Monome() {
	close();
}

Monome::Monome(Monome&& other) noexcept
	: monome_(other.monome_),
	  callbacks_(std::move(other.callbacks_)) {
	other.monome_ = nullptr;
	rebindCallbacks();
}

Monome& Monome::operator=(Monome&& other) noexcept {
	if( this == &other )
		return *this;

	close();

	monome_ = other.monome_;
	callbacks_ = std::move(other.callbacks_);
	other.monome_ = nullptr;
	rebindCallbacks();

	return *this;
}

monome_t* Monome::raw() const noexcept {
	return monome_;
}

std::string Monome::serial() const {
	return stringFromNullable(monome_get_serial(handle()));
}

std::string Monome::devpath() const {
	return stringFromNullable(monome_get_devpath(handle()));
}

std::string Monome::friendlyName() const {
	return stringFromNullable(monome_get_friendly_name(handle()));
}

std::string Monome::proto() const {
	return stringFromNullable(monome_get_proto(handle()));
}

int Monome::rows() const {
	return monome_get_rows(handle());
}

int Monome::cols() const {
	return monome_get_cols(handle());
}

void Monome::setRotation(Rotation rotation) {
	monome_set_rotation(handle(), toC(rotation));
}

Rotation Monome::rotation() const {
	return rotationFromC(monome_get_rotation(handle()));
}

int Monome::ledSet(unsigned int x, unsigned int y, bool on) {
	return monome_led_set(handle(), x, y, on ? 1 : 0);
}

int Monome::ledOn(unsigned int x, unsigned int y) {
	return monome_led_on(handle(), x, y);
}

int Monome::ledOff(unsigned int x, unsigned int y) {
	return monome_led_off(handle(), x, y);
}

int Monome::ledAll(bool on) {
	return monome_led_all(handle(), on ? 1 : 0);
}

int Monome::ledMap(unsigned int xOff, unsigned int yOff,
                   const std::array<std::uint8_t, 8>& data) {
	return ledMap(xOff, yOff, data.data());
}

int Monome::ledMap(unsigned int xOff, unsigned int yOff,
                   const std::uint8_t* data) {
	return monome_led_map(handle(), xOff, yOff, data);
}

int Monome::ledCol(unsigned int x, unsigned int yOff, std::size_t count,
                   const std::uint8_t* colData) {
	return monome_led_col(handle(), x, yOff, count, colData);
}

int Monome::ledRow(unsigned int xOff, unsigned int y, std::size_t count,
                   const std::uint8_t* rowData) {
	return monome_led_row(handle(), xOff, y, count, rowData);
}

int Monome::ledIntensity(unsigned int brightness) {
	return monome_led_intensity(handle(), brightness);
}

int Monome::ledLevelSet(unsigned int x, unsigned int y, unsigned int level) {
	return monome_led_level_set(handle(), x, y, level);
}

int Monome::ledLevelAll(unsigned int level) {
	return monome_led_level_all(handle(), level);
}

int Monome::ledLevelMap(unsigned int xOff, unsigned int yOff,
                        const std::array<std::uint8_t, 64>& data) {
	return ledLevelMap(xOff, yOff, data.data());
}

int Monome::ledLevelMap(unsigned int xOff, unsigned int yOff,
                        const std::uint8_t* data) {
	return monome_led_level_map(handle(), xOff, yOff, data);
}

int Monome::ledLevelRow(unsigned int xOff, unsigned int y, std::size_t count,
                        const std::uint8_t* data) {
	return monome_led_level_row(handle(), xOff, y, count, data);
}

int Monome::ledLevelCol(unsigned int x, unsigned int yOff, std::size_t count,
                        const std::uint8_t* data) {
	return monome_led_level_col(handle(), x, yOff, count, data);
}

int Monome::ledRingSet(unsigned int ring, unsigned int led,
                       unsigned int level) {
	return monome_led_ring_set(handle(), ring, led, level);
}

int Monome::ledRingAll(unsigned int ring, unsigned int level) {
	return monome_led_ring_all(handle(), ring, level);
}

int Monome::ledRingMap(unsigned int ring,
                       const std::array<std::uint8_t, 64>& levels) {
	return ledRingMap(ring, levels.data());
}

int Monome::ledRingMap(unsigned int ring, const std::uint8_t* levels) {
	return monome_led_ring_map(handle(), ring, levels);
}

int Monome::ledRingRange(unsigned int ring, unsigned int start,
                         unsigned int end, unsigned int level) {
	return monome_led_ring_range(handle(), ring, start, end, level);
}

int Monome::ledRingIntensity(unsigned int brightness) {
	return monome_led_ring_intensity(handle(), brightness);
}

int Monome::tiltEnable(unsigned int sensor) {
	return monome_tilt_enable(handle(), sensor);
}

int Monome::tiltDisable(unsigned int sensor) {
	return monome_tilt_disable(handle(), sensor);
}

int Monome::eventNext(Event& out) {
	monome_event_t event{};
	const int status = monome_event_next(handle(), &event);
	if( status > 0 )
		out = Event(event);

	return status;
}

int Monome::eventHandleNext() {
	monome_event_t event{};
	const int status = monome_event_next(handle(), &event);
	if( status <= 0 )
		return status;

	std::size_t index = 0;
	if( !eventIndex(event.event_type, index) || !callbacks_[index] )
		return 0;

	Event wrapped(event);
	callbacks_[index](wrapped);
	return 1;
}

void Monome::eventLoop() {
	monome_event_loop(handle());
}

void Monome::on(EventType type, Callback callback) {
	auto* monome = handle();
	const auto index = eventIndex(type);
	callbacks_[index] = std::move(callback);

	const auto cType = toC(type);
	const int status = callbacks_[index]
		? monome_register_handler(monome, cType, &Monome::handleEvent, this)
		: monome_unregister_handler(monome, cType);

	if( status )
		throw std::runtime_error("failed to update monome event handler");
}

monome_t* Monome::handle() const {
	if( !monome_ )
		throw std::runtime_error("monome handle is not open");

	return monome_;
}

void Monome::close() {
	if( !monome_ )
		return;

	unregisterCallbacks();
	monome_close(monome_);
	monome_ = nullptr;
}

void Monome::unregisterCallbacks() noexcept {
	for( std::size_t i = 0; i < callbacks_.size(); ++i )
		monome_unregister_handler(monome_, static_cast<monome_event_type_t>(i));
}

void Monome::rebindCallbacks() noexcept {
	if( !monome_ )
		return;

	for( std::size_t i = 0; i < callbacks_.size(); ++i ) {
		if( callbacks_[i] )
			monome_register_handler(monome_, static_cast<monome_event_type_t>(i),
			                        &Monome::handleEvent, this);
	}
}

void Monome::dispatch(const monome_event_t& event) {
	std::size_t index = 0;
	if( !eventIndex(event.event_type, index) || !callbacks_[index] )
		return;

	Event wrapped(event);
	callbacks_[index](wrapped);
}

void Monome::handleEvent(const monome_event_t* event, void* data) noexcept {
	try {
		if( event && data )
			static_cast<Monome*>(data)->dispatch(*event);
	} catch( ... ) {
		std::terminate();
	}
}

} // namespace monomepp
