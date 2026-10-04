"""Web server for the Docker image (README.md, "Docker").

Serves the browser build, the Warblade data mounted at /data (read-only) and the save folder
mounted at /saves, which the page keeps in sync with the game's /save folder:

  GET  /                      the page, warblade.js, warblade.wasm
  GET  /api/data              {"files": [{"path", "size"}]} of the data folder (404: no data)
  GET  /data/<path>           a data file
  GET  /api/saves             {"files": [{"path", "size"}]} of the save folder
  GET  /saves/<path>          a save file
  PUT  /saves/<path>          writes a save file (atomically)
  DELETE /saves/<path>        removes a save file

Settings come from the environment:
  PORT                the port to listen on (default 8080)

Standard library only.
"""
import json
import mimetypes
import os
import sys
import tempfile
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import unquote, urlsplit

WEB_DIR = os.environ.get("WARBLADE_WEB", "/app/web")
DATA_MOUNT = os.environ.get("WARBLADE_DATA", "/data")
SAVES_DIR = os.environ.get("WARBLADE_SAVES", "/saves")
PORT = int(os.environ.get("PORT", "8080"))
MAX_SAVE_BYTES = 16 * 1024 * 1024

mimetypes.add_type("application/wasm", ".wasm")
mimetypes.add_type("text/javascript", ".js")


def find_data_dir():
    """The folder holding warblade.pac: the mount itself (the install's data/ folder was
    mounted) or its data/ subfolder (the whole install was). Names match in any case."""
    for candidate in (DATA_MOUNT, os.path.join(DATA_MOUNT, "data"), os.path.join(DATA_MOUNT, "Data")):
        try:
            if any(name.lower() == "warblade.pac" for name in os.listdir(candidate)):
                return candidate
        except OSError:
            pass
    return None


def list_files(root):
    files = []
    for folder, dirs, names in os.walk(root):
        dirs.sort()
        for name in sorted(names):
            full = os.path.join(folder, name)
            if name.startswith(".") or not os.path.isfile(full):
                continue
            files.append({"path": os.path.relpath(full, root).replace(os.sep, "/"),
                          "size": os.path.getsize(full)})
    return files


def safe_join(root, rel):
    """`rel` inside `root`, or None if it would leave it."""
    rel = unquote(rel).replace("\\", "/").lstrip("/")
    if not rel or any(part in ("", ".", "..") for part in rel.split("/")):
        return None
    full = os.path.realpath(os.path.join(root, rel))
    real_root = os.path.realpath(root)
    if os.path.commonpath([full, real_root]) != real_root:
        return None
    return full


class Handler(SimpleHTTPRequestHandler):
    server_version = "WarbladeSR"

    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=WEB_DIR, **kwargs)

    def log_message(self, fmt, *args):
        sys.stderr.write("%s %s\n" % (self.address_string(), fmt % args))

    # ---- helpers ----

    def send_plain(self, code, text=""):
        body = text.encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "text/plain; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        if self.command != "HEAD":
            self.wfile.write(body)

    def send_json(self, value):
        body = json.dumps(value).encode("utf-8")
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        if self.command != "HEAD":
            self.wfile.write(body)

    def send_file(self, path, cache):
        """A file with Last-Modified, answering If-Modified-Since with 304."""
        try:
            f = open(path, "rb")
        except OSError:
            self.send_plain(404, "not found")
            return
        with f:
            st = os.fstat(f.fileno())
            since = self.headers.get("If-Modified-Since")
            if since and cache:
                try:
                    from email.utils import parsedate_to_datetime
                    if int(st.st_mtime) <= parsedate_to_datetime(since).timestamp():
                        self.send_response(304)
                        self.end_headers()
                        return
                except (TypeError, ValueError, OverflowError):
                    pass
            self.send_response(200)
            self.send_header("Content-Type", self.guess_type(path))
            self.send_header("Content-Length", str(st.st_size))
            self.send_header("Last-Modified", self.date_time_string(st.st_mtime))
            self.send_header("Cache-Control", "no-cache" if cache else "no-store")
            self.end_headers()
            if self.command != "HEAD":
                self.copyfile(f, self.wfile)

    # ---- methods ----

    def do_GET(self):
        path = urlsplit(self.path).path
        if path == "/api/data":
            data = find_data_dir()
            if data is None:
                return self.send_plain(404, "no Warblade data mounted at " + DATA_MOUNT)
            return self.send_json({"files": list_files(data)})
        if path == "/api/saves":
            return self.send_json({"files": list_files(SAVES_DIR)})
        if path.startswith("/data/"):
            data = find_data_dir()
            full = safe_join(data, path[len("/data/"):]) if data else None
            return self.send_file(full, cache=True) if full else self.send_plain(404, "not found")
        if path.startswith("/saves/"):
            full = safe_join(SAVES_DIR, path[len("/saves/"):])
            return self.send_file(full, cache=False) if full else self.send_plain(404, "not found")
        return super().do_GET()

    def do_HEAD(self):
        self.do_GET()

    def do_PUT(self):
        path = urlsplit(self.path).path
        full = safe_join(SAVES_DIR, path[len("/saves/"):]) if path.startswith("/saves/") else None
        if full is None:
            return self.send_plain(404, "not found")
        try:
            length = int(self.headers.get("Content-Length", "-1"))
        except ValueError:
            length = -1
        if length < 0 or length > MAX_SAVE_BYTES:
            return self.send_plain(413, "bad or too large Content-Length")
        body = self.rfile.read(length)
        os.makedirs(os.path.dirname(full), exist_ok=True)
        fd, tmp = tempfile.mkstemp(dir=os.path.dirname(full), prefix=".upload-")
        try:
            with os.fdopen(fd, "wb") as out:
                out.write(body)
            os.replace(tmp, full)
        except OSError as e:
            try:
                os.unlink(tmp)
            except OSError:
                pass
            return self.send_plain(500, "could not save: %s" % e)
        self.send_plain(204)

    def do_DELETE(self):
        path = urlsplit(self.path).path
        full = safe_join(SAVES_DIR, path[len("/saves/"):]) if path.startswith("/saves/") else None
        if full is None:
            return self.send_plain(404, "not found")
        try:
            os.unlink(full)
        except FileNotFoundError:
            pass
        except OSError as e:
            return self.send_plain(500, "could not delete: %s" % e)
        self.send_plain(204)

    def end_headers(self):
        # The page and the game code change with each image; don't let browsers keep stale ones.
        if urlsplit(self.path).path in ("/", "/index.html", "/warblade.js", "/warblade.wasm"):
            self.send_header("Cache-Control", "no-cache")
        super().end_headers()


def main():
    os.makedirs(SAVES_DIR, exist_ok=True)
    data = find_data_dir()
    print("Warblade SR on port %d; data: %s; saves: %s" % (
        PORT, data or "none (players choose their own folder)", SAVES_DIR), flush=True)
    ThreadingHTTPServer(("", PORT), Handler).serve_forever()


if __name__ == "__main__":
    main()
