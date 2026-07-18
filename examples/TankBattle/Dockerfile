# Minimal atlasnetsdk stage
FROM archlinux:latest AS runner-dev
WORKDIR /atlasnet
RUN pacman -Syu --noconfirm && \
    pacman -S --noconfirm \
        bash \
        glibc \
        gcc-libs \
        zlib \
        openssl \
        brotli \
        zstd \
        binutils \
        gdb && \
    pacman -Scc --noconfirm

FROM scratch AS tankbattlestage
COPY /.stage /tankbattle


FROM runner-dev AS tankbattle-shard-dev
COPY --from=tankbattlestage /tankbattle/TankBattleShard /tankbattle/TankBattleShard
ARG PRELAUNCH_CMD="{EXE}"
ENV PRELAUNCH_CMD="${PRELAUNCH_CMD}"
ENTRYPOINT ["sh", "-c", "EXE=/tankbattle/TankBattleShard; CMD=$(printf '%s' \"$PRELAUNCH_CMD\" | sed \"s|{EXE}|$EXE|g\"); exec sh -c \"$CMD\""]
