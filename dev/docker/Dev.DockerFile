# Minimal atlasnetsdk stage
FROM debian:trixie-slim AS runner
WORKDIR /atlasnet
RUN apt-get update && apt-get install -y libatomic1 binutils gdb


FROM scratch AS atlasnetstage
COPY ./dev/.stage /atlasnet

FROM scratch AS atlasnet-scr
COPY ./src /atlasnet/src
COPY ./include /atlasnet/include


FROM runner AS atlasnet-controller-dev
COPY --from=atlasnetstage /atlasnet/AtlasNetController /atlasnet/AtlasNetController
ARG PRELAUNCH_CMD="{EXE}"
ENV PRELAUNCH_CMD="${PRELAUNCH_CMD}"
RUN chmod +x /atlasnet/AtlasNetController
ENTRYPOINT ["sh", "-c", "EXE=/atlasnet/AtlasNetController; CMD=$(printf '%s' \"$PRELAUNCH_CMD\" | sed \"s|{EXE}|$EXE|g\"); exec sh -c \"$CMD\""]


FROM runner AS atlasnet-agent-dev
COPY --from=atlasnetstage /atlasnet/AtlasNetAgent /atlasnet/AtlasNetAgent
ARG PRELAUNCH_CMD="{EXE}"
ENV PRELAUNCH_CMD="${PRELAUNCH_CMD}"
ENTRYPOINT ["sh", "-c", "EXE=/atlasnet/AtlasNetAgent; CMD=$(printf '%s' \"$PRELAUNCH_CMD\" | sed \"s|{EXE}|$EXE|g\"); exec sh -c \"$CMD\""]


FROM runner AS atlasnet-proxy-dev
COPY --from=atlasnetstage /atlasnet/AtlasNetProxy /atlasnet/AtlasNetProxy
ARG PRELAUNCH_CMD="{EXE}"
ENV PRELAUNCH_CMD="${PRELAUNCH_CMD}"
ENTRYPOINT ["sh", "-c", "EXE=/atlasnet/AtlasNetProxy; CMD=$(printf '%s' \"$PRELAUNCH_CMD\" | sed \"s|{EXE}|$EXE|g\"); exec sh -c \"$CMD\""]

FROM runner AS atlasnet-dummy-dev
COPY --from=atlasnetstage /atlasnet/AtlasNetDummy /atlasnet/AtlasNetDummy
ARG PRELAUNCH_CMD="{EXE}"
ENV PRELAUNCH_CMD="${PRELAUNCH_CMD}"
ENTRYPOINT ["sh", "-c", "EXE=/atlasnet/AtlasNetDummy; CMD=$(printf '%s' \"$PRELAUNCH_CMD\" | sed \"s|{EXE}|$EXE|g\"); exec sh -c \"$CMD\""]

FROM node:lts-alpine AS atlasnet-cartograph-frontend-dev
# -------------------------
# Frontend build
# -------------------------
COPY --from=atlasnet-scr /atlasnet/src/web/frontend /atlasnet/AtlasNetCartographFrontend
WORKDIR /atlasnet/AtlasNetCartographFrontend
RUN npm install && npm run build
ENV FRONTEND_PORT=4173
ENTRYPOINT npm run preview -- --host --port $FRONTEND_PORT


FROM runner AS atlasnet-cartograph-backend-dev
RUN apt-get update && apt-get install -y libjsoncpp-dev


# -------------------------
# Backend
# -------------------------
WORKDIR /
COPY --from=atlasnetstage /atlasnet/AtlasNetCartographBackend /atlasnet/AtlasNetCartographBackend

ENTRYPOINT ["/atlasnet/AtlasNetCartographBackend"]

FROM nginx:alpine AS atlasnet-cartograph-proxy-dev
RUN apk add --no-cache gettext

COPY --from=atlasnet-scr /atlasnet/src/web/proxy/nginx.conf.template /etc/nginx/nginx.conf.template
COPY --from=atlasnet-scr /atlasnet/src/web/proxy/entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh
ENTRYPOINT ["/entrypoint.sh"]