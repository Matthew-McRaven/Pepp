self.addEventListener('fetch', event => {
    event.respondWith(fetch(event.request).then(response => {
        const clonedResponse = response.clone();
        if (response.status === 200 || response.type === 'opaque') {
            caches.open('wasm_cache').then(cache => cache.put(event.request, clonedResponse)).catch(err => console.warn('Cache put failed:', err));
        }
        return response; // return original
    }).catch(() => {
        return caches.match(event.request).then(match => match || Response.error());
    }));
});
