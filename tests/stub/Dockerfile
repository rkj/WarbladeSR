# Builds the stand-in game for the browser tests (tests/stub/stub.c) with the same Emscripten
# and link options as the real one (Dockerfile, CMakeLists.txt). Output: warblade.js and
# warblade.wasm (docker build --output type=local,dest=DIR tests/stub).
FROM emscripten/emsdk:4.0.23 AS build
COPY stub.c /src/stub.c
RUN mkdir /out && emcc -O1 /src/stub.c -o /out/warblade.js \
        -sASYNCIFY -sASYNCIFY_STACK_SIZE=131072 -sSTACK_SIZE=4MB \
        -sALLOW_MEMORY_GROWTH -sINITIAL_MEMORY=256MB \
        -sINVOKE_RUN=0 -sEXIT_RUNTIME=1 -sFORCE_FILESYSTEM -lidbfs.js \
        "-sEXPORTED_RUNTIME_METHODS=['FS','callMain']"

FROM scratch
COPY --from=build /out/ /
