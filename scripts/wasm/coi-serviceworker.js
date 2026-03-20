/*! coi-service-worker v0.1.7 - Guido Zuidhof and contributors, licensed under MIT */
/*
 * Cross-Origin Isolation Service Worker
 *
 * This service worker intercepts all fetch responses and adds the
 * Cross-Origin-Opener-Policy and Cross-Origin-Embedder-Policy headers
 * required for SharedArrayBuffer (multi-threaded WebAssembly).
 *
 * Deploy this file alongside the WASM application HTML files.
 * It is automatically registered by the <script> snippet in each .html page.
 *
 * Source: https://github.com/nickvdp/coi-serviceworker (MIT)
 * Adapted for MNE-CPP WASM deployment on GitHub Pages.
 */
let coepCredentialless = false;

if (typeof window === "undefined") {
    // --- Service Worker scope ---
    self.addEventListener("install", () => self.skipWaiting());
    self.addEventListener("activate", (event) =>
        event.waitUntil(self.clients.claim())
    );

    self.addEventListener("message", (ev) => {
        if (ev.data && ev.data.type === "deregister") {
            self.registration
                .unregister()
                .then(() => self.clients.matchAll())
                .then((clients) => {
                    clients.forEach((client) => client.navigate(client.url));
                });
        }
    });

    self.addEventListener("fetch", function (event) {
        const r = event.request;
        if (r.cache === "only-if-cached" && r.mode !== "same-origin") {
            return;
        }

        event.respondWith(
            fetch(r).then((response) => {
                if (response.status === 0) {
                    return response;
                }

                const newHeaders = new Headers(response.headers);
                newHeaders.set("Cross-Origin-Embedder-Policy",
                    coepCredentialless ? "credentialless" : "require-corp"
                );
                newHeaders.set("Cross-Origin-Opener-Policy", "same-origin");

                return new Response(response.body, {
                    status: response.status,
                    statusText: response.statusText,
                    headers: newHeaders,
                });
            }).catch((e) => console.error(e))
        );
    });
} else {
    // --- Window scope (registration logic) ---
    (() => {
        const reloadedByCOI = window.sessionStorage.getItem("coiReloadedByCOI");
        window.sessionStorage.removeItem("coiReloadedByCOI");

        const coiError = () => {
            console.error(
                "[COI] Service worker registration failed. " +
                "Is the service worker file served from the same origin?"
            );
        };

        // Already cross-origin isolated — nothing to do
        if (window.crossOriginIsolated !== false) {
            return;
        }

        // Safari does not yet support credentialless
        if (!window.isSecureContext) {
            !reloadedByCOI &&
                console.log(
                    "[COI] Requires a secure context (HTTPS or localhost)."
                );
            return;
        }

        // Feature-detect credentialless mode (Chrome 96+)
        if (
            !/Firefox/.test(navigator.userAgent) &&
            "credentialless" in XMLHttpRequest.prototype
        ) {
            coepCredentialless = true;
        }

        // Register the service worker
        if (navigator.serviceWorker) {
            navigator.serviceWorker
                .register(new URL("coi-serviceworker.js", window.location.href).href)
                .then(
                    (registration) => {
                        registration.addEventListener("updatefound", () => {
                            // If the service worker is already installed and has control,
                            // we are likely seeing a subsequent load.  Skip early exit.
                            if (
                                registration.installing &&
                                !navigator.serviceWorker.controller
                            ) {
                                registration.installing.addEventListener(
                                    "statechange",
                                    function () {
                                        if (this.state === "activated") {
                                            // First activation — reload to pick up the new headers
                                            window.sessionStorage.setItem(
                                                "coiReloadedByCOI",
                                                "true"
                                            );
                                            window.location.reload();
                                        }
                                    }
                                );
                            }
                        });
                    },
                    coiError
                );
        } else {
            coiError();
        }
    })();
}
