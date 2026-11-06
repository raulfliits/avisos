// Service worker de FLiiTS Catalunya World: guarda el juego para abrirlo sin conexión.
// Solo controla FLiiTS_Catalunya_World.html (scope), no el resto de avisos.
const CACHE = "cw-v1";
const FILES = ["FLiiTS_Catalunya_World.html", "catalunya-world.webmanifest", "cw-icon-192.png", "cw-icon-512.png", "cw-apple-touch-icon.png"];
self.addEventListener("install", e => { e.waitUntil(caches.open(CACHE).then(c => c.addAll(FILES)).then(() => self.skipWaiting())); });
self.addEventListener("activate", e => { e.waitUntil(caches.keys().then(ks => Promise.all(ks.filter(k => k.startsWith("cw-") && k !== CACHE).map(k => caches.delete(k)))).then(() => self.clients.claim())); });
// Red primero (así siempre tienes la última versión) y, si no hay conexión, la copia guardada.
self.addEventListener("fetch", e => {
  if (e.request.method !== "GET") return;
  e.respondWith(fetch(e.request).then(r => { if (r.ok) { const cp = r.clone(); caches.open(CACHE).then(c => c.put(e.request, cp)); } return r; })
    .catch(() => caches.match(e.request, { ignoreSearch: true })));
});
