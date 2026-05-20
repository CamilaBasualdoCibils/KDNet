# Minimal atlasnetsdk stage
FROM debian:trixie-slim AS runner
WORKDIR /atlasnet
RUN apt-get update && apt-get install -y libatomic1 binutils gdb

FROM runner AS web-runner
RUN apt-get update && apt-get install -y libjsoncpp-dev


FROM scratch AS atlasnetstage
COPY ./dev/.stage /atlasnet
FROM scratch AS atlasnetstage-cartograph-frontend
COPY ./web .

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

FROM web-runner AS atlasnet-cartograph-dev
RUN apt-get update && apt-get install -y curl ca-certificates gnupg \
 && curl -fsSL https://deb.nodesource.com/setup_current.x | bash - \
 && apt-get install -y nodejs \
 && node -v && npm -v
COPY --from=atlasnetstage-cartograph-frontend ./package.json /atlasnet/AtlasNetCartographFrontend/package.json
WORKDIR /atlasnet/AtlasNetCartographFrontend
RUN npm install -g pnpm@latest-11
RUN pnpm install


COPY --from=atlasnetstage-cartograph-frontend . /atlasnet/AtlasNetCartographFrontend
WORKDIR /
COPY --from=atlasnetstage /atlasnet/AtlasNetCartographBackend /atlasnet/AtlasNetCartographBackend
ARG PRELAUNCH_CMD="{EXE}"
ENV PRELAUNCH_CMD="${PRELAUNCH_CMD}"
WORKDIR /atlasnet/AtlasNetCartographFrontend
ENTRYPOINT [ "pnpm","run", "start" ]
#ENTRYPOINT ["sh", "-c", "EXE=/atlasnet/AtlasNetCartographBackend; CMD=$(printf '%s' \"$PRELAUNCH_CMD\" | sed \"s|{EXE}|$EXE|g\"); exec sh -c \"$CMD\""]