# Minimal atlasnetsdk stage
FROM debian:trixie-slim AS runner
WORKDIR /atlasnet
RUN apt-get update && apt-get install -y libatomic1 binutils gdb build-essential

FROM scratch AS tankbattlestage
COPY /.stage /tankbattle


FROM runner AS tankbattle-shard-dev
COPY --from=tankbattlestage /tankbattle/TankBattleShard /tankbattle/TankBattleShard
ARG PRELAUNCH_CMD="{EXE}"
ENV PRELAUNCH_CMD="${PRELAUNCH_CMD}"
ENTRYPOINT ["sh", "-c", "EXE=/tankbattle/TankBattleShard; CMD=$(printf '%s' \"$PRELAUNCH_CMD\" | sed \"s|{EXE}|$EXE|g\"); exec sh -c \"$CMD\""]
