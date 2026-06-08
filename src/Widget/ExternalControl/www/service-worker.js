// 函数级注释：Service Worker - 缓存静态资源以提升慢网速下的加载与刷新体验
const CACHE_NAME = 'ns-cache-v16';
const CORE_URLS = [
  'index.html',
  'setting.html',
  'favicon.png',
  'assets/vendor/bootstrap.min.css',
  'assets/vendor/index.css',
  'assets/ns-compat.css',
  'assets/vendor/bootstrap.bundle.min.js',
  'assets/vendor/vue.global.prod.js',
  'assets/vendor/index.full.min.js',
  'assets/vendor/split.min.js',
  'assets/widgets.js',
  'assets/ep-widgets.js',
  'assets/widget-manifest.js',
  'assets/modules/ns-namespace.js',
  'assets/modules/ns-layout-persist.js',
  'assets/modules/ns-canvas.js',
  'assets/modules/ns-auth.js',
  'assets/modules/ns-ws-sync.js',
  'assets/modules/ns-utils.js',
  'assets/modules/ns-a11y.js',
  'assets/modules/ns-prop-schema.js',
  'assets/modules/ns-ws-status.js',
  'assets/modules/ns-widget-library.js',
  'assets/modules/ns-interact.js',
  'assets/modules/ns-dashboard-ctx.js',
  'assets/modules/ns-dashboard-history.js',
  'assets/modules/ns-dashboard-layout.js',
  'assets/modules/ns-dashboard-prop-panel.js',
  'assets/modules/ns-dashboard-selection.js',
  'assets/modules/ns-dashboard-edit.js',
  'assets/modules/ns-dashboard-tabs.js',
  'assets/modules/ns-dashboard.js'
];

const WIDGET_URLS = [
  'widgets/Button/widget.js', 'widgets/Button/widget.html',
  'widgets/Slider/widget.js', 'widgets/Slider/widget.html',
  'widgets/FloatSlider/widget.js', 'widgets/FloatSlider/widget.html',
  'widgets/VSlider/widget.js', 'widgets/VSlider/widget.html',
  'widgets/VFloatSlider/widget.js', 'widgets/VFloatSlider/widget.html',
  'widgets/Checkbox/widget.js', 'widgets/Checkbox/widget.html',
  'widgets/Switch/widget.js', 'widgets/Switch/widget.html',
  'widgets/Input/widget.js', 'widgets/Input/widget.html',
  'widgets/ToggleButton/widget.js', 'widgets/ToggleButton/widget.html',
  'widgets/Divider/widget.js', 'widgets/Divider/widget.html',
  'widgets/VDivider/widget.js', 'widgets/VDivider/widget.html',
  'widgets/Label/widget.js', 'widgets/Label/widget.html',
  'widgets/Knob/widget.js', 'widgets/Knob/widget.html',
  'widgets/TimeCode/widget.js', 'widgets/TimeCode/widget.html',
  'widgets/Timeline/widget.js', 'widgets/Timeline/widget.html',
  'widgets/Number/widget.js', 'widgets/Number/widget.html',
  'widgets/Frame/widget.js', 'widgets/Frame/widget.html',
  'widgets/Hyperlink/widget.js', 'widgets/Hyperlink/widget.html'
];

const URLS_TO_CACHE = CORE_URLS.concat(WIDGET_URLS);

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

  if (url.protocol !== 'http:' && url.protocol !== 'https:') {
    return;
  }
  if (url.origin !== self.location.origin) {
    return;
  }

  event.respondWith((async () => {
    const cache = await caches.open(CACHE_NAME);
    const accept = req.headers.get('accept') || '';
    const isHtmlNav = req.mode === 'navigate' || accept.includes('text/html');

    if (isHtmlNav) {
      try {
        const res = await fetch(req);
        if (res && res.ok) {
          event.waitUntil(cache.put(req, res.clone()));
        }
        return res;
      } catch {
        const cached = await cache.match(req);
        if (cached) return cached;
        return new Response('离线不可用', { status: 503, headers: { 'Content-Type': 'text/plain; charset=utf-8' } });
      }
    }

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
