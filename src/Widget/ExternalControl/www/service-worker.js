// 函数级注释：Service Worker - 缓存静态资源以提升慢网速下的加载与刷新体验
const CACHE_NAME = 'ns-cache-v1';
const URLS_TO_CACHE = [
  'favicon.png',
  'assets/vendor/bootstrap.min.css',
  'assets/vendor/index.css',
  'assets/vendor/bootstrap.bundle.min.js',
  'assets/vendor/vue.global.prod.js',
  'assets/vendor/index.full.min.js',
  'assets/vendor/split.min.js',
  'assets/widgets.js',
  'assets/ep-widgets.js',
  'widgets/Button/widget.js',
  'widgets/Slider/widget.js',
  'widgets/FloatSlider/widget.js',
  'widgets/VSlider/widget.js',
  'widgets/VFloatSlider/widget.js',
  'widgets/Checkbox/widget.js',
  'widgets/Switch/widget.js',
  'widgets/Input/widget.js',
  'widgets/ToggleButton/widget.js',
  'widgets/Divider/widget.js',
  'widgets/VDivider/widget.js',
  'widgets/Label/widget.js',
  'widgets/Knob/widget.js',
  'widgets/Timecode/widget.js',
  'widgets/Timeline/widget.js',
  'widgets/Number/widget.js',
  'widgets/Frame/widget.js'
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
  if (req.method !== 'GET') return;
  event.respondWith(
    caches.match(req).then((cached) => {
      if (cached) {
        fetch(req).then((res) => {
          if (res && res.ok) caches.open(CACHE_NAME).then((c) => c.put(req, res.clone()));
        }).catch(() => {});
        return cached;
      }
      return fetch(req).then((res) => {
        if (res && res.ok) caches.open(CACHE_NAME).then((c) => c.put(req, res.clone()));
        return res;
      }).catch(() => cached || new Response('', { status: 504 }));
    })
  );
});