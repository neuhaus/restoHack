/* curses.h stub for the Emscripten build. wasm/stub-include is on the
 * include path only for that build (see CMakeLists.txt).
 *
 * Emscripten ships no curses and the terminal is xterm.js, so the game
 * makes no curses calls. The one symbol these sources expect from a
 * curses header - delay_output() - is declared in wasm/wasm.h and
 * implemented in wasm/wasm.c. Keep this stub declaration-free so the
 * real declarations stay in one place.
 */
#ifndef RESTOHACK_STUB_CURSES_H
#define RESTOHACK_STUB_CURSES_H
#endif
