import http.server, sys
class H(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        body = b'''<!DOCTYPE html><html><head><title>t</title><script>
addEventListener("pageshow", e => { window.persistedShow = e.persisted; });
window.rafCount = 0; function tick() { window.rafCount++; requestAnimationFrame(tick); } requestAnimationFrame(tick);
</script></head><body style="background:linear-gradient(red,blue);height:3000px"><h1>page</h1>
<div style="width:200px;height:200px;background:green;will-change:transform;animation:spin 2s linear infinite"></div>
<style>@keyframes spin { to { transform: rotate(360deg); } }</style></body></html>'''
        self.send_response(200)
        self.send_header('Content-Type', 'text/html')
        if 'coop' in self.path:
            self.send_header('Cross-Origin-Opener-Policy', 'same-origin-allow-popups')
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        self.wfile.write(body)
    def log_message(self, *a): pass
http.server.ThreadingHTTPServer(('0.0.0.0', int(sys.argv[1])), H).serve_forever()
