# Warblade SR in the browser, served with the game data and saves on the server (README.md,
# "Docker"). The image has no game data: mount your Warblade 1.34 data folder at /data.
#
#   docker build -t warblade-sr .
#   docker run -p 8080:8080 -v /path/to/Warblade/data:/data:ro -v warblade-saves:/saves warblade-sr

# ---- build: the WebAssembly game (build-web.sh) ----
FROM emscripten/emsdk:4.0.23 AS build
# The project needs CMake 3.25+ and Ninja; the image has CMake 3.22 and no Ninja.
RUN pip3 install --no-cache-dir "cmake>=3.25" ninja
WORKDIR /src
COPY . .
RUN ./build-web.sh

# ---- run: a small Python server for the page, the data and the saves ----
FROM python:3.12-alpine
RUN adduser -D -u 1000 warblade && mkdir -p /app/web /data /saves && chown warblade /saves
COPY --from=build /src/build/web/index.html /src/build/web/warblade.js /src/build/web/warblade.wasm /app/web/
COPY docker/server.py /app/server.py
USER warblade
ENV PORT=8080
EXPOSE 8080
VOLUME ["/saves"]
HEALTHCHECK --interval=30s --timeout=5s CMD wget -q -O /dev/null "http://127.0.0.1:${PORT}/healthz" || exit 1
CMD ["python3", "/app/server.py"]
