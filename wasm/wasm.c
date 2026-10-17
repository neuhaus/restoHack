/* wasm.c - WebAssembly terminal and POSIX shims for restoHack (Hack 1.0.3)
 *
 * MODERN ADDITION (2026): Implements the declarations in wasm/wasm.h.
 *
 * The terminal side provides the termcap entry points hack.termcap.c
 * expects, translated to ANSI escapes that xterm.js understands. Actual
 * I/O (bytes in/out, termios raw mode, window size, Ctrl+C) is handled by
 * the xterm-pty bridge between Emscripten and xterm.js - see
 * wasm/shell.html and wasm/README.md. This file deliberately compiles
 * (as harmless no-op shims) with a native compiler too, so the stranded
 * logic stays syntax-checked without an Emscripten SDK on hand.
 */

#include "wasm.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define WASM_SLEEP(ms) emscripten_sleep(ms)
#else
/* Native syntax-check fallback: no browser event loop to yield to. */
#define WASM_SLEEP(ms) ((void)0)
#endif

#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* termcap for xterm.js: one fixed terminal, 80 columns by 24 rows.    */
/* ------------------------------------------------------------------ */

int wasm_tgetent(char *bp, const char *name) {
  (void)bp;
  (void)name;
  return 1; /* exactly one terminal type: the xterm.js canvas */
}

char *wasm_tgetstr(const char *id, char **area) {
  (void)area;

  /* Capability strings startup() asks for, as ANSI escapes. Anything the
   * game never queries returns NULL (capability absent), exactly like a
   * minimal real termcap entry would. */
  static char cm[] = "CM"; /* cursor motion: expanded by wasm_tgoto() */
  static char cl[] = "\033[H\033[2J";
  static char ho[] = "\033[H";
  static char ce[] = "\033[K";
  static char cd[] = "\033[J";
  static char up[] = "\033[A";
  static char nd[] = "\033[C";
  static char bc[] = "\010";
  static char so[] = "\033[7m";
  static char se[] = "\033[27m";

  if (strcmp(id, "cm") == 0)
    return cm;
  if (strcmp(id, "cl") == 0)
    return cl;
  if (strcmp(id, "ho") == 0)
    return ho;
  if (strcmp(id, "ce") == 0)
    return ce;
  if (strcmp(id, "cd") == 0)
    return cd;
  if (strcmp(id, "up") == 0)
    return up;
  if (strcmp(id, "nd") == 0)
    return nd;
  if (strcmp(id, "bc") == 0)
    return bc;
  if (strcmp(id, "so") == 0)
    return so;
  if (strcmp(id, "se") == 0)
    return se;
  return NULL; /* pc, xd, ...: absent, caller has fallbacks */
}

int wasm_tgetnum(const char *id) {
  if (strcmp(id, "co") == 0)
    return 80;
  if (strcmp(id, "li") == 0)
    return 24;
  return -1; /* sg, ...: absent */
}

int wasm_tgetflag(const char *id) {
  (void)id;
  return 0; /* NP, nx, bs, os: all absent */
}

char *wasm_tgoto(const char *cap, int col, int row) {
  static char buf[32];
  (void)cap; /* only ever called for CM */
  /* tgoto takes 0-based columns/rows; ANSI CUP wants 1-based. */
  (void)snprintf(buf, sizeof(buf), "\033[%d;%dH", row + 1, col + 1);
  return buf;
}

void wasm_tputs(const char *str, int affcnt, int (*outc)(int)) {
  (void)affcnt; /* no padding accounting: our strings carry no $<...> */
  if (str == NULL)
    return; /* TI/TE/VS/VE are unset; xputs() still calls us with them */
  while (*str)
    outc((unsigned char)*str++);
}

/*
 * Pause for effect (monster flashes, zap beams). Yielding to the browser
 * event loop also lets pending terminal output flush to xterm.js, which is
 * exactly what the xterm-pty bridge recommends instead of busy-waiting.
 */
int delay_output(int ms) {
  if (ms > 0)
    WASM_SLEEP((unsigned int)ms);
  return 0;
}
