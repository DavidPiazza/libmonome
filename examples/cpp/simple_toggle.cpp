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

/**
 * simple_toggle.cpp
 * press a button to toggle it!
 */

#include <array>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

#include <monomepp/Monome.hpp>

namespace {

constexpr const char* MonomeDevice = "osc.udp://127.0.0.1:8080/monome";
constexpr const char* ListenPort = "8000";

} // namespace

int main(int argc, char* argv[]) {
	const std::string device = argc > 1 ? argv[1] : MonomeDevice;
	std::array<std::array<bool, 16>, 16> grid{};

	try {
		monomepp::Monome monome(device, ListenPort);
		monome.ledAll(false);

		monome.on(monomepp::EventType::ButtonDown,
		          [&](const monomepp::Event& event) {
			          const auto x = event.gridX();
			          const auto y = event.gridY();

			          if( x >= grid.size() || y >= grid[x].size() )
				          return;

			          grid[x][y] = !grid[x][y];
			          monome.ledSet(x, y, grid[x][y]);
		          });

		monome.eventLoop();
	} catch( const std::exception& e ) {
		std::cerr << e.what() << '\n';
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
