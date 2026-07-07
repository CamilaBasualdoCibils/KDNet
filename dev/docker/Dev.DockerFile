# Minimal AtlasNet runtime stage
FROM archlinux:latest AS runner

WORKDIR /atlasnet

RUN pacman -Syu --noconfirm && \
    pacman -S --noconfirm \
        bash \
        glibc \
        gcc-libs \
        zlib \
        openssl \
        c-ares \
        jsoncpp \
        util-linux-libs \
        brotli \
        sqlite \
        zstd \
        binutils \
        gdb && \
    pacman -Scc --noconfirm

FROM scratch AS atlasnetstage
COPY ./dev/.stage /atlasnet

FROM scratch AS atlasnet-src
COPY ./src /atlasnet/src
COPY ./include /atlasnet/include


FROM runner AS atlasnet-controller-dev
COPY --from=atlasnetstage /atlasnet/AtlasNetController /atlasnet/AtlasNetController
ARG PRELAUNCH_CMD="{EXE}"
ENV PRELAUNCH_CMD="${PRELAUNCH_CMD}"
RUN chmod +x /atlasnet/AtlasNetController
ENTRYPOINT ["bash", "-c", "EXE=/atlasnet/AtlasNetController; CMD=$(printf '%s' \"$PRELAUNCH_CMD\" | sed \"s|{EXE}|$EXE|g\"); exec bash -c \"$CMD\""]


FROM runner AS atlasnet-gateway-dev
COPY --from=atlasnetstage /atlasnet/AtlasNetGateway /atlasnet/AtlasNetGateway
ARG PRELAUNCH_CMD="{EXE}"
ENV PRELAUNCH_CMD="${PRELAUNCH_CMD}"
RUN chmod +x /atlasnet/AtlasNetGateway
ENTRYPOINT ["bash", "-c", "EXE=/atlasnet/AtlasNetGateway; CMD=$(printf '%s' \"$PRELAUNCH_CMD\" | sed \"s|{EXE}|$EXE|g\"); exec bash -c \"$CMD\""]


#FROM runner AS atlasnet-dummy-dev
#COPY --from=atlasnetstage /atlasnet/AtlasNetDummy /atlasnet/AtlasNetDummy
#ARG PRELAUNCH_CMD="{EXE}"
#ENV PRELAUNCH_CMD="${PRELAUNCH_CMD}"
#RUN chmod +x /atlasnet/AtlasNetDummy
#ENTRYPOINT ["bash", "-c", "EXE=/atlasnet/AtlasNetDummy; CMD=$(printf '%s' \"$PRELAUNCH_CMD\" | sed \"s|{EXE}|$EXE|g\"); exec bash -c \"$CMD\""]


FROM runner AS atlasnet-cartograph-backend-dev

WORKDIR /

COPY --from=atlasnetstage /atlasnet/AtlasNetCartographBackend /atlasnet/AtlasNetCartographBackend

RUN chmod +x /atlasnet/AtlasNetCartographBackend

ENTRYPOINT ["/atlasnet/AtlasNetCartographBackend"]


FROM nginx:alpine AS atlasnet-cartograph-frontend-dev

COPY --from=atlasnet-src /atlasnet/src/web/frontend /atlasnet/AtlasNetCartographFrontend

WORKDIR /atlasnet/AtlasNetCartographFrontend

RUN apk add --no-cache nodejs npm gettext

RUN npm install && npm run build

COPY --from=atlasnet-src /atlasnet/src/web/nginx/nginx.conf.template /etc/nginx/nginx.conf.template
COPY --from=atlasnet-src /atlasnet/src/web/nginx/entrypoint.sh /entrypoint.sh

RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]