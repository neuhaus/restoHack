# restoHack in the browser (WebAssembly port)

Play Hack 1.0.3 (1985) in a web page: the shared C sources compile with
Emscripten to `hack.mjs`/`hack.wasm` and run against an [xterm.js]
terminal through the [xterm-pty] bridge. No game logic is forked or
rewritten for the web — the port is a thin, upstreamable layer.

## Tools you need installed

| Tool | Why | Get it |
| ---- | --- | ------ |
| Emscripten SDK (`emcc`, `emcmake`) | the wasm compiler | `git clone https://github.com/emscripten-core/emsdk.git`, then `./emsdk install latest && ./emsdk activate latest` and `source ./emsdk_env.sh` (xterm-pty 0.12.0 was verified against emsdk 3.1.47; any recent SDK should work — report otherwise) |
| CMake (≥3.16), a host C compiler (`cc`) | configure + host-built `makedefs` | distro package (`cmake`, `build-essential`/`base-devel`) |
| Node.js | ships with emsdk tooling | distro package or nodejs.org |
| Python 3 (`http.server`) | serve the build locally (any static server works) | distro package |
| A modern browser | run it | Chrome/Firefox/Safari, current |

The build needs network access: it fetches the pinned browser assets
(xterm.js, xterm-pty, and xterm-pty's Emscripten glue) from unpkg and
verifies their SHA-256s. The built bundle references only local files,
so the page itself plays fully offline.

## Build and run

```bash
./wasm/build-wasm.sh
cd build/wasm && python3 -m http.server 8000
# open http://localhost:8000/
```

What the script does: fetches the pinned, checksum-verified assets
(`xterm.js`/`xterm.css`/`xterm-pty.mjs` into the bundle dir,
`emscripten-pty.js` for the link step), builds in `build/wasm-build/`
(the CMake working dir), and stages `index.html` plus
`hack.mjs`/`hack.wasm`/`hack.data` into `build/wasm/` - a self-contained
directory holding only the files needed to run: serve it (or copy it
anywhere) and play offline. No special headers (COOP/COEP) are needed:
the build uses Asyncify on the main thread, not pthreads.

## How it works (the small diff)

The game already abstracts the terminal through termcap (`startup()`,
`cmov()`, `xputs()`…) and reads everything through `getchar()`. The
browser port plugs into exactly those two seams:

- **Terminal output** — `wasm/wasm.c` implements the six termcap entry
  points (`tgetent`, `tgetstr`, `tgetnum`, `tgetflag`, `tgoto`, `tputs`)
  as fixed ANSI escapes for an 80×24 screen, plus `delay_output()` that
  yields to the browser event loop so animations flush. Bytes flow
  game → Emscripten stdout → xterm-pty → xterm.js.
- **Keyboard input** — blocking reads (`getchar()` in `readchar()`,
  `getlin()`, `askname()`, …) pause the wasm module via `-sASYNCIFY=1`
  and resume on the next keypress through xterm-pty's `fd_read` hook.
  Raw mode (`cbreak`, no echo) is the game's own `termios` code:
  xterm-pty serves the underlying `TCGETS`/`TCSETS` ioctls, so
  `hack.tty.c`/`hack.ioctl.c` needed only include-path nudges, not
  rewrites. Window size (`TIOCGWINSZ`) and resize/`Ctrl+C` come from the
  bridge the same way.
- **Files** — the playground is preloaded at the FS root `/`
  (`--preload-file` of the normal `setup_hackdir` output: `data`, `help`,
  `hh`, `rumors`, `news`, `record`, `perm`), and the game runs there (no
  chdir). `/save` is additionally mounted on IDBFS by `wasm/pre.js`, so
  save files survive reloads: any flush starts only after the initial
  load has succeeded (an un-restored directory must never reconcile
  over IndexedDB), saves then flush every 5 s and on page hide, and the
  page does a final awaited flush when the game exits.
- **Shell-visible saves** — `index.html` uses the exported `FS`
  (`-sEXPORTED_RUNTIME_METHODS=FS`) to snapshot `/save` at startup
  and diff it on exit: since save and death both exit 0, a new or changed
  save file is what tells the page "Game saved (name)"; otherwise it reports
  the dungeon claimed you. The same listing drives the "Saved games: …"
  line (player names are the save filenames minus the numeric uid prefix),
  so returning players know which name to type at the prompt. No game code
  involved — pure shell logic.
- **No POSIX shims needed** — probes against the SDK show Emscripten
  already provides `flock()`/`fcntl()` (which succeed), `fork`/`exec`/`wait`
  (which link; the pager paths using them are never taken since the
  internal pager is always used and there is no shell), and the identity
  calls — so the game links and runs with no impersonation layer.
  already provides `flock()`/`fcntl()` (which succeed), `fork`/`exec`/`wait`
  (which link; the pager paths using them are never taken since the
  internal pager is always used and there is no shell), and the identity
  calls — so the game links and runs with no impersonation layer.

Touched game sources (each a few lines, all `#ifdef __EMSCRIPTEN__`):

- `src/compat.h` — inject `wasm.h` (same pattern as the Windows shim).
- `src/config.h` — `#undef SUSPEND` (no `SIGTSTP` in a tab).
- `src/hack.tty.c`, `src/hack.ioctl.c` — take the `termios` path.
- `src/hack.unix.c` — `gethdate()` short-circuits (`argv[0]` is the
  virtual `./this.program` and can never `stat`; saves are versioned).
- `CMakeLists.txt` — `if(EMSCRIPTEN)` blocks: no curses/ELF flags,
  host-built `makedefs`, `wasm/wasm.c` source, stub `<curses.h>`/
  `<termcap.h>` on the include path, `HAVE_DELAY_OUTPUT` set (wasm.c
  provides `delay_output()`), Asyncify/FS/preload link flags, `.mjs`
  output. Native builds are unaffected.

## Known v1 limitations

- High scores (`record`), ghost files (`bones_xx`) and lock files live in
  MEMFS: they last for the tab session but not across reloads. Only
  `/save` persists (IndexedDB). Full Filesystem persistence is a
  natural follow-up.
- One game per tab; two tabs sharing saves can race (Emscripten's `flock()`
  succeeds trivially — the Web Locks API would be the real fix).
- `HACKOPTIONS`/`HACKDIR` env vars and `-d` are meaningless in the
  browser. Every player is asked for a name at startup (the Emscripten
  default identity is deliberately ignored). Fixed 80×24 canvas
  (larger xterm sizes are reported through, and the resize path redraws,
  but the dungeon layout stays 80×24).
- Death/end screens call `exit()` — the page shows a "reload to delve
  again" note via `onExit`.

## Verifying (for the reviewer with emsdk)

1. `./wasm/build-wasm.sh && cd build/wasm && python3 -m http.server 8000`
2. Open `http://localhost:8000/`: name prompt appears; `hjkl` moves `@`; `?` help,
   `/` what-is, `S` save, `Q` quit all work through the internal pager.
3. Save (`S`): the page reports "Game saved (name)" by diffing the save
   listing (save and death share exit 0); reload, retype the name, and the
   save restores from `/save`.
4. Die once: tombstone shows, score line prints, status reports the dungeon
   claimed you.
5. Native regression: plain `cmake -S . -B build && cmake --build build`
   is untouched by the wasm guards (no `__EMSCRIPTEN__` defined).

[xterm.js]: https://github.com/xtermjs/xterm.js
[xterm-pty]: https://github.com/mame/xterm-pty
