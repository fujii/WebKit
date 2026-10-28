import http.server, sys
PORT = int(sys.argv[1])
A = b'''<!DOCTYPE html><html><head><script>
window.rafCount = 0; function tick() { window.rafCount++; requestAnimationFrame(tick); } requestAnimationFrame(tick);
addEventListener("pageshow", e => {
  if (!e.persisted) { setTimeout(() => { location.href = "http://127.0.0.2:%d/b"; }, 2000); return; }
  const start = window.rafCount;
  setTimeout(() => { fetch("/report?persisted=1&raf=" + (window.rafCount - start)); }, 1000);
});
</script></head><body style="background:linear-gradient(red,blue);height:3000px"><h1>A</h1>
<div style="width:200px;height:200px;background:green;will-change:transform;animation:spin 2s linear infinite"></div>
<style>@keyframes spin { to { transform: rotate(360deg); } }</style></body></html>''' % PORT
B = b'''<!DOCTYPE html><html><body style="background:yellow"><h1>B</h1><script>
addEventListener("load", () => setTimeout(() => { fetch("/report?b=1").finally(() => history.back()); }, 6000));
</script></body></html>'''
class H(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        body = A if self.path.startswith('/a') else B if self.path.startswith('/b') else b'ok'
        if self.path.startswith('/report'):
            print(self.path, flush=True)
        self.send_response(200)
        self.send_header('Content-Type', 'text/html')
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        self.wfile.write(body)
    def log_message(self, *a): pass
http.server.ThreadingHTTPServer(('0.0.0.0', PORT), H).serve_forever()
