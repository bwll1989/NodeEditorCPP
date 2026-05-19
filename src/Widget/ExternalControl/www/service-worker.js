// 函数级注释：Service Worker - 缓存静态资源以提升慢网速下的加载与刷新体验
const CACHE_NAME = 'ns-cache-v3';
const URLS_TO_CACHE = [
  'index.html',
  'setting.html',
  'favicon.png',
  'assets/vendor/bootstrap.min.css',
  'assets/vendor/index.css',
  'assets/vendor/bootstrap.bundle.min.js',
  'assets/vendor/vue.global.prod.js',
  'assets/vendor/index.full.min.js',
  'assets/vendor/split.min.js',
  'assets/widgets.js',
  'assets/ep-widgets.js',
  'assets/modules/ns-namespace.js',
  'assets/modules/ns-canvas.js',
  'assets/modules/ns-ws-sync.js',
  'assets/modules/ns-utils.js',
  'assets/modules/ns-interact.js',
  'assets/modules/ns-dashboard.js'
];

self.addEventListener('install', (event) => {
  event.waitUntil(
    caches.open(CACHE_NAME).then((cache) => cache.addAll(URLS_TO_CACHE)).then(() => self.skipWaiting())
  );
});

self.addEventListener('activate', (event) => {
  event.waitUntil(
    caches.keys().then((keys) =>
      Promise.all(keys.map((k) => (k === CACHE_NAME ? Promise.resolve() : caches.delete(k))))
    ).then(() => self.clients.claim())
  );
});

self.addEventListener('fetch', (event) => {
  const req = event.request;
  if (!req || req.method !== 'GET') return;

  let url;
  try {
    url = new URL(req.url);
  } catch {
    return;
  }

  // 函数级注释：仅缓存同源 http(s) 请求，避免 chrome-extension:// 等协议导致 Cache.put 报错。
  if (url.protocol !== 'http:' && url.protocol !== 'https:') {
    return;
  }
  if (url.origin !== self.location.origin) {
    return;
  }

  event.respondWith((async () => {
    const cache = await caches.open(CACHE_NAME);
    const cached = await cache.match(req);

    if (cached) {
      event.waitUntil((async () => {
        try {
          const res = await fetch(req);
          if (res && res.ok) {
            await cache.put(req, res.clone());
          }
        } catch {}
      })());
      return cached;
    }

    try {
      const res = await fetch(req);
      if (res && res.ok) {
        event.waitUntil(cache.put(req, res.clone()));
      }
      return res;
    } catch {
      return new Response('', { status: 504 });
    }
  })());
});