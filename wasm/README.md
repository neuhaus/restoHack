# restoHack in the browser (WebAssembly port)

Play Hack 1.0.3 (1985) in a web page.
Use Emscripten to compile the C sources `hack.mjs`/`hack.wasm` and 
run it against an [xterm.js] terminal through the [xterm-pty] bridge. 

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

Enjoy!

---

[xterm.js]: https://github.com/xtermjs/xterm.js
[xterm-pty]: https://github.com/mame/xterm-pty
