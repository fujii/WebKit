# Usage: chain-server.py <port>
# /coop, /plain: test pages (COOP same-origin-allow-popups on /coop).
# /chain/<n>/<last>: COOP page on 127.0.0.<n> that navigates to /chain/<n+1>/<last> on 127.0.0.<n+1> after 2 s.
# /go204/<n>: plain page on 127.0.0.1 that navigates to 127.0.0.<n>/nocontent (204, the navigation is cancelled).
import http.server, sys
PORT = int(sys.argv[1])
STYLE = b'''<body style="background:linear-gradient(red,blue);height:3000px"><h1>page</h1>
<div style="width:200px;height:200px;background:green;will-change:transform;animation:spin 2s linear infinite"></div>
<style>@keyframes spin { to { transform: rotate(360deg); } }</style>'''
def page(script=b''):
    return b'<!DOCTYPE html><html><head><script>' + script + b'</script></head>' + STYLE + b'</body></html>'
class H(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        p = self.path.split('/')
        coop = True
        if self.path.startswith('/nocontent'):
            self.send_response(204); self.send_header('Content-Length', '0'); self.end_headers(); return
        if self.path.startswith('/chain/'):
            n, last = int(p[2]), int(p[3])
            body = page(b'setTimeout(() => { location.href = "http://127.0.0.%d:%d/chain/%d/%d"; }, 2000);' % (n + 1, PORT, n + 1, last) if n < last else b'')
        elif self.path.startswith('/go204/'):
            coop = False
            body = page(b'setTimeout(() => { location.href = "http://127.0.0.%d:%d/nocontent"; }, 2000);' % (int(p[2]), PORT))
        else:
            coop = 'coop' in self.path
            body = page()
        self.send_response(200)
        self.send_header('Content-Type', 'text/html')
        if coop:
            self.send_header('Cross-Origin-Opener-Policy', 'same-origin-allow-popups')
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        self.wfile.write(body)
    def log_message(self, *a): pass
http.server.ThreadingHTTPServer(('0.0.0.0', PORT), H).serve_forever()
