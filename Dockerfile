# syntax=docker/dockerfile:1.7

ARG EMSDK_IMAGE=emscripten/emsdk:latest
ARG NGINX_IMAGE=nginx:alpine
ARG ALPINE_IMAGE=alpine:3.22

FROM ${EMSDK_IMAGE} AS emscripten-build
WORKDIR /src

COPY . .

ARG CMAKE_BUILD_TYPE=RelWithDebInfo
ARG RENEGADE_EMSCRIPTEN_BUILD_DIR=/src/build-docker-em
ARG RENEGADE_EMSCRIPTEN_DATA_ROOT=/src/Renegade

RUN --mount=type=cache,target=/root/.cache/emscripten \
    emcmake cmake -S /src -B "${RENEGADE_EMSCRIPTEN_BUILD_DIR}" \
        -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}" \
        -DRENEGADE_EMSCRIPTEN_PACKAGE_GAME_DATA=ON \
        -DRENEGADE_EMSCRIPTEN_LAZY_FETCH_GAME_DATA=ON \
        -DRENEGADE_EMSCRIPTEN_DATA_ROOT="${RENEGADE_EMSCRIPTEN_DATA_ROOT}" \
        -DRENEGADE_EMSCRIPTEN_ALLOW_MEMORY_GROWTH=ON \
    && cmake --build "${RENEGADE_EMSCRIPTEN_BUILD_DIR}" -j20

FROM ${ALPINE_IMAGE} AS emscripten-bundle
ARG RENEGADE_EMSCRIPTEN_BUILD_DIR=/src/build-docker-em
WORKDIR /bundle

COPY --from=emscripten-build ${RENEGADE_EMSCRIPTEN_BUILD_DIR}/bin/ ./

RUN rm -rf Renegade-assets

FROM ${ALPINE_IMAGE} AS game-data
WORKDIR /work

COPY Renegade/ ./Renegade/

RUN mkdir -p /bundle/Data \
    && cp -a Renegade/Data/. /bundle/Data/ \
    && for dir in HTML Internet; do \
        if [ -d "Renegade/${dir}" ]; then \
            mkdir -p "/bundle/${dir}" \
            && cp -a "Renegade/${dir}/." "/bundle/${dir}/"; \
        fi; \
    done \
    && find Renegade -mindepth 1 -maxdepth 1 -type f \
        ! -iname '*.exe' \
        ! -iname '*.dll' \
        ! -iname '*.asi' \
        ! -iname '*.m3d' \
        ! -iname '*.bmp' \
        ! -iname '*.ico' \
        ! -iname '*.doc' \
        ! -iname '*.xml' \
        ! -iname '*.vdf' \
        -exec cp -a {} /bundle/ \;

FROM ${NGINX_IMAGE} AS nginx-runtime-base

RUN apk add --no-cache apache2-utils

COPY docker/nginx/30-renegade-nginx.sh /docker-entrypoint.d/30-renegade-nginx.sh
RUN chmod +x /docker-entrypoint.d/30-renegade-nginx.sh

ENV HTTP_BASIC_AUTH_USERNAME= \
    HTTP_BASIC_AUTH_PASSWORD= \
    HTTP_BASIC_AUTH_REALM=Renegade \
    NGINX_PORT=80

EXPOSE 80

FROM nginx-runtime-base AS engine

RUN mkdir -p /usr/share/nginx/html/Renegade-assets
COPY --from=emscripten-bundle /bundle/ /usr/share/nginx/html/

FROM nginx-runtime-base AS with-data

COPY --from=game-data /bundle/ /usr/share/nginx/html/Renegade-assets/
COPY --from=emscripten-bundle /bundle/ /usr/share/nginx/html/
