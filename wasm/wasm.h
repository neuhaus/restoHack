/* wasm.h - WebAssembly/browser compatibility layer for restoHack (Hack 1.0.3)
 *
 * MODERN ADDITION (2026): Emscripten port of the 1985 game to the browser.
 *
 * WHY: Emscripten (clang/wasm) provides no curses or termcap library. The
 * terminal itself is supplied by xterm.js through the xterm-pty bridge (see
 * wasm/README.md), which speaks ANSI escapes and intercepts the usual
 * termios ioctls - so the game only needs the termcap *entry points*
 * (tgetent/tgetstr/...) defined. Everything else the game needs (flock,
 * termios, signals, files) Emscripten already provides.
 *
 * HOW: Included from src/compat.h when __EMSCRIPTEN__ is defined (same
 * injection pattern as the Windows port's win32.h). The termcap functions
 * and shims below are implemented in wasm/wasm.c. The stub <curses.h>/
 * <termcap.h> headers in wasm/stub-include satisfy those includes for the
 * Emscripten build, so game sources need no wasm-specific guards.
 *
 * PRESERVES: All game logic untouched. The termcap contract (capability
 * strings, tgoto cursor motion, 80x24 screen) matches what startup() in
 * hack.termcap.c expects from a real termcap entry.
 */

#ifndef RESTOHACK_WASM_H
#define RESTOHACK_WASM_H

#ifdef __EMSCRIPTEN__

/*
 * Termcap entry points for xterm.js.
 *
 * Implemented in wasm/wasm.c as fixed ANSI escape sequences. tgetent()
 * always succeeds (there is exactly one terminal: the xterm.js canvas),
 * tgetnum("co")/("li") report the canonical 80x24 the dungeon is drawn for.
 */
int wasm_tgetent(char *bp, const char *name);
char *wasm_tgetstr(const char *id, char **area);
int wasm_tgetnum(const char *id);
int wasm_tgetflag(const char *id);
char *wasm_tgoto(const char *cap, int col, int row);
void wasm_tputs(const char *str, int affcnt, int (*outc)(int));
#define tgetent wasm_tgetent
#define tgetstr wasm_tgetstr
#define tgetnum wasm_tgetnum
#define tgetflag wasm_tgetflag
#define tgoto wasm_tgoto
#define tputs wasm_tputs

/* delay_output() is defined in wasm/wasm.c (yields to the browser event
 * loop so animation frames flush to the screen). The fallback definition
 * in src/hack.termcap.c is excluded here because the build sets
 * HAVE_DELAY_OUTPUT. */
extern int delay_output(int ms);

/* NOTE: no POSIX shims live here. Probes against the Emscripten SDK show it
 * already provides flock/fcntl (which succeed), kill/link/setuid/setgid/
 * usleep (which link; the fork/exec/wait paths are never taken since the
 * internal pager is always used and there is no shell), plus getuid/getpid/
 * umask/chdir/dup - so the game needs no identity/locking shims at all. */

#endif /* __EMSCRIPTEN__ */
#endif /* RESTOHACK_WASM_H */
