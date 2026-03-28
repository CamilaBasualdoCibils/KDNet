# Minimal atlasnetsdk stage
FROM debian:trixie-slim AS runner
WORKDIR /atlasnet
RUN apt-get update && apt-get install -y libatomic1 binutils gdb build-essential

FROM runner AS web-runner
RUN apt-get update && apt-get install -y libjsoncpp-dev


FROM scratch AS atlasnetstage
COPY ./dev/.stage /atlasnet


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

FROM web-runner AS atlasnet-web-dev
COPY --from=atlasnetstage /atlasnet/AtlasNetWebBackend /atlasnet/AtlasNetWebBackend
ARG PRELAUNCH_CMD="{EXE}"
ENV PRELAUNCH_CMD="${PRELAUNCH_CMD}"
ENTRYPOINT ["sh", "-c", "EXE=/atlasnet/AtlasNetWebBackend; CMD=$(printf '%s' \"$PRELAUNCH_CMD\" | sed \"s|{EXE}|$EXE|g\"); exec sh -c \"$CMD\""]