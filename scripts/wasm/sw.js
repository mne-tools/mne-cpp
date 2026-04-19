// MNE Inspect Service Worker
// Combines COOP/COEP header injection with offline caching
const CACHE_VERSION = 'mne-inspect-v2.2.0';

const PRECACHE_ASSETS = [
  './',
  './mne_inspect.html',
  './mne_inspect.js',
  './mne_inspect.wasm',
  './mne_inspect.data',
  './qtloader.js',
  './qtlogo.svg',
  './manifest.json'
];

// Install: precache core assets
self.addEventListener('install', (event) => {
  event.waitUntil(
    caches.open(CACHE_VERSION).then((cache) => {
      return cache.addAll(PRECACHE_ASSETS);
    })
  );
  self.skipWaiting();
});

// Activate: clean old caches
self.addEventListener('activate', (event) => {
  event.waitUntil(
    caches.keys().then((keys) => {
      return Promise.all(
        keys.filter((key) => key !== CACHE_VERSION)
            .map((key) => caches.delete(key))
      );
    })
  );
  self.clients.claim();
});

// Fetch: cache-first for immutable assets, network-first for HTML
self.addEventListener('fetch', (event) => {
  const url = new URL(event.request.url);

  // Inject COOP/COEP headers for SharedArrayBuffer support
  if (event.request.mode === 'navigate') {
    event.respondWith(
      fetch(event.request).then((response) => {
        const headers = new Headers(response.headers);
        headers.set('Cross-Origin-Opener-Policy', 'same-origin');
        headers.set('Cross-Origin-Embedder-Policy', 'require-corp');
        return new Response(response.body, {
          status: response.status,
          statusText: response.statusText,
          headers
        });
      }).catch(() => {
        return caches.match(event.request);
      })
    );
    return;
  }

  // Cache-first for .wasm, .data, .js (large, immutable per version)
  const isImmutable = url.pathname.endsWith('.wasm') ||
                      url.pathname.endsWith('.data') ||
                      url.pathname.endsWith('.js');

  if (isImmutable) {
    event.respondWith(
      caches.match(event.request).then((cached) => {
        if (cached) return cached;
        return fetch(event.request).then((response) => {
          const headers = new Headers(response.headers);
          headers.set('Cross-Origin-Embedder-Policy', 'require-corp');
          headers.set('Cross-Origin-Resource-Policy', 'same-origin');
          const modifiedResponse = new Response(response.body, {
            status: response.status,
            statusText: response.statusText,
            headers
          });
          const clone = modifiedResponse.clone();
          caches.open(CACHE_VERSION).then((cache) => cache.put(event.request, clone));
          return modifiedResponse;
        });
      })
    );
    return;
  }

  // Default: network with CORP headers
  event.respondWith(
    fetch(event.request).then((response) => {
      const headers = new Headers(response.headers);
      headers.set('Cross-Origin-Embedder-Policy', 'require-corp');
      headers.set('Cross-Origin-Resource-Policy', 'same-origin');
      return new Response(response.body, {
        status: response.status,
        statusText: response.statusText,
        headers
      });
    }).catch(() => caches.match(event.request))
  );
});
