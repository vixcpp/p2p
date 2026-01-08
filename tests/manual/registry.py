# registry.py
from http.server import BaseHTTPRequestHandler, HTTPServer
import json, time

PEERS = {}  # node_id -> {host,tcp_port,ts}

class H(BaseHTTPRequestHandler):
    def _json(self, code, obj):
        b = json.dumps(obj).encode()
        self.send_response(code)
        self.send_header("Content-Type","application/json")
        self.send_header("Content-Length", str(len(b)))
        self.end_headers()
        self.wfile.write(b)

    def do_GET(self):
        if self.path.startswith("/p2p/v1/peers"):
            peers = []
            for v in PEERS.values():
                peers.append({"host": v["host"], "tcp_port": v["tcp_port"], "node_id": v["node_id"]})
            self._json(200, {"peers": peers[:20]})
            return
        self._json(404, {"error":"not found"})

    def do_POST(self):
        if self.path == "/p2p/v1/announce":
            n = int(self.headers.get("Content-Length","0"))
            body = self.rfile.read(n).decode("utf-8","ignore")
            try:
                obj = json.loads(body)
                node_id = obj.get("node_id","")
                tcp_port = int(obj.get("tcp_port",0))
                if not node_id or tcp_port <= 0: raise ValueError()
                host = self.client_address[0]
                PEERS[node_id] = {"node_id":node_id, "host":host, "tcp_port":tcp_port, "ts":int(time.time()*1000)}
                self._json(200, {"ok":True})
            except:
                self._json(400, {"ok":False})
            return
        self._json(404, {"error":"not found"})

HTTPServer(("0.0.0.0",8080), H).serve_forever()
