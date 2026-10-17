/* wasm/pre.js - runs inside the Emscripten module before main().
 *
 * The static playground (data, help, rumors, record, ...) is preloaded by
 * the linker into MEMFS at the root "/", where the game runs (no chdir),
 * so its save games live at /save. That directory is mounted on IDBFS
 * (browser IndexedDB) here so a saved game survives a page reload.
 *
 * Startup blocks on the initial sync from IndexedDB. Periodic flushes
 * only start once that load has succeeded: an un-restored (empty)
 * directory must never be reconciled over IndexedDB, or one failed read
 * would delete every stored save. index.html performs one final awaited
 * flush when the game exits, so a saved game is durable before the page
 * suggests a reload.
 *
 * NOTE: this file is concatenated into the module by --pre-js, so `Module`,
 * `FS`, `IDBFS` and `ENV` are already in scope. Do not redeclare them.
 */
Module["preRun"] = Module["preRun"] || [];
Module["preRun"].push(function () {
  // The browser has no login identity (Emscripten defaults USER/LOGNAME to
  // "web_user"): unset both so the game's name lookup comes up empty and
  // it asks the player for a name.
  if (typeof ENV !== "undefined") {
    ENV.USER = undefined;
    ENV.LOGNAME = undefined;
  }

  try {
    FS.mkdir("/save");
  } catch (e) {}

  try {
    FS.mount(IDBFS, {}, "/save");
  } catch (e) {
    // No IndexedDB (private browsing, non-browser runtime, ...): the game
    // still works, saves just last for this session (MEMFS).
    console.error("restoHack: persistent saves unavailable:", e);
    Module["savesPersistent"] = false;
    return;
  }
  Module["savesPersistent"] = true;

  // One flush is one IndexedDB transaction; the guard keeps flushes
  // sequential. A flush that does not finish within 15s (its transaction
  // blocked behind one from a page being torn down, ...) is abandoned so
  // later flushes still run.
  var syncing = false,
    syncStarted = 0;
  function flushSaves() {
    if (syncing && Date.now() - syncStarted < 15000)
      return;
    syncing = true;
    syncStarted = Date.now();
    FS.syncfs(false, function (err) {
      syncing = false;
      if (err)
        console.error("restoHack: could not flush saves to IndexedDB:", err);
    });
  }
  // Promise variant for index.html: wait out any in-flight flush, then
  // perform a final one. Resolves with the error (null on success).
  Module["flushSaves"] = function () {
    return new Promise(function (resolve) {
      (function go() {
        if (syncing && Date.now() - syncStarted < 15000)
          return setTimeout(go, 200);
        syncing = true;
        syncStarted = Date.now();
        FS.syncfs(false, function (err) {
          syncing = false;
          resolve(err);
        });
      })();
    });
  };

  // Initial sync from IndexedDB; main() waits on this run dependency.
  try {
    Module["addRunDependency"]("wasm-idbfs-load");
    FS.syncfs(true, function (err) {
      Module["removeRunDependency"]("wasm-idbfs-load");
      if (err) {
        // Stored saves could not be read. Keep flushes disabled for this
        // session: the directory is not a faithful copy of IndexedDB, and
        // reconciling it would delete the stored saves, which stay intact
        // for the next load.
        console.error(
          "restoHack: could not load saved games from IndexedDB:",
          err
        );
        Module["savesPersistent"] = false;
        return;
      }
      setInterval(flushSaves, 5000);
      if (typeof window !== "undefined" && window.addEventListener)
        window.addEventListener("pagehide", flushSaves);
    });
  } catch (e) {
    console.error("restoHack: persistent saves unavailable:", e);
    Module["savesPersistent"] = false;
    try {
      Module["removeRunDependency"]("wasm-idbfs-load");
    } catch (e2) {}
  }
});
