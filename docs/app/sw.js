// Service worker for the S'mores Line visualizer.
//
// Its job: keep the LARGE, immutable in-browser C++ toolchain (clang+lld+sysroot,
// ~60 MB under vendor/wasm-clang/) in the Cache API so the Studio's C++ tab
// downloads it once and serves it from local storage on every later visit —
// instant, and working offline. App assets use network-first so a redeploy is
// never stale online, but still work offline after the first visit. Cross-origin
// CDN assets (Monaco, Pyodide) are left to the browser's HTTP cache.
//
// Bump CACHE to invalidate everything (e.g. if the vendored toolchain changes).
const CACHE = "smores-line-v1";
const TOOLCHAIN = /\/vendor\/wasm-clang\//;   // large + immutable -> cache-first

self.addEventListener("install", (e) => { self.skipWaiting(); });

self.addEventListener("activate", (e) => {
  e.waitUntil((async () => {
    const keys = await caches.keys();
    await Promise.all(keys.filter((k) => k !== CACHE).map((k) => caches.delete(k)));
    await self.clients.claim();
  })());
});

self.addEventListener("fetch", (e) => {
  const req = e.request;
  if (req.method !== "GET") return;
  const url = new URL(req.url);
  if (url.origin !== self.location.origin) return;   // CDN assets: use the HTTP cache

  if (TOOLCHAIN.test(url.pathname)) {
    // cache-first: the toolchain blobs are huge and never change for a given version
    e.respondWith((async () => {
      const cache = await caches.open(CACHE);
      const hit = await cache.match(req);
      if (hit) return hit;
      const res = await fetch(req);
      if (res.ok) cache.put(req, res.clone());
      return res;
    })());
    return;
  }

  // app shell (html / wasm / layouts / meta): network-first, fall back to cache
  e.respondWith((async () => {
    const cache = await caches.open(CACHE);
    try {
      const res = await fetch(req);
      if (res.ok) cache.put(req, res.clone());
      return res;
    } catch (err) {
      const hit = await cache.match(req);
      if (hit) return hit;
      throw err;
    }
  })());
});
