/* termcap.h stub for the Emscripten build. wasm/stub-include is on the
 * include path only for that build (see CMakeLists.txt).
 *
 * Emscripten ships no termcap library. The six termcap entry points
 * (tgetent/tgetstr/tgetnum/tgetflag/tgoto/tputs) are #defined to the
 * wasm_tget* implementations and declared in wasm/wasm.h; keep this
 * stub declaration-free so the declarations stay in one place.
 */
#ifndef RESTOHACK_STUB_TERMCAP_H
#define RESTOHACK_STUB_TERMCAP_H
#endif
