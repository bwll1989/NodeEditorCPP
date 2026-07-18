// 函数级注释：Service Worker - 缓存静态资源以提升慢网速下的加载与刷新体验
const CACHE_NAME = 'ns-cache-v18';
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
  'assets/style.css',
  'assets/widgets-sfc.js',
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

const URLS_TO_CACHE = CORE_URLS;

self.addEventListener('install', (event) => {
  event.waitUntil(
    caches.open(CACHE_NAME).then(async (cache) => {
      // 逐个预热，避免 addAll 因单个 404 导致整批失败，也降低 install 时对服务器的并发冲击
      await Promise.all(
        URLS_TO_CACHE.map(async (url) => {
          try {
            await cache.add(url);
          } catch {}
        })
      );
    }).then(() => self.skipWaiting())
  );
});

self.addEventListener('activate', (event) => {
  event.waitUntil(
    caches.keys().then((keys) =>
      Promise.all(keys.map((k) => (k === CACHE_NAME ? Promise.resolve() : caches.delete(k))))
    ).then(() => self.clients.claim())
  );
});

async function matchCachedHtml(cache, req) {
  const hit = await cache.match(req);
  if (hit) return hit;
  try {
    const url = new URL(req.url);
    if (url.pathname === '/' || url.pathname === '') {
      return (await cache.match('index.html')) || (await cache.match('/index.html'));
    }
  } catch {}
  return null;
}

function revalidateInBackground(cache, req) {
  return (async () => {
    try {
      const res = await fetch(req);
      if (res && res.ok) {
        await cache.put(req, res.clone());
      }
    } catch {}
  })();
}

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

    // HTML 导航：cache-first + 后台更新（避免 network-first 在 C++ 服务繁忙时阻塞 20s）
    if (isHtmlNav) {
      const cached = await matchCachedHtml(cache, req);
      if (cached) {
        event.waitUntil(revalidateInBackground(cache, req));
        return cached;
      }
      try {
        const res = await fetch(req);
        if (res && res.ok) {
          event.waitUntil(cache.put(req, res.clone()));
        }
        return res;
      } catch {
        return new Response('离线不可用', { status: 503, headers: { 'Content-Type': 'text/plain; charset=utf-8' } });
      }
    }

    const cached = await cache.match(req);
    if (cached) {
      event.waitUntil(revalidateInBackground(cache, req));
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
