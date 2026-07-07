FROM debian:trixie-slim AS builder
WORKDIR /atlasnet
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    pkg-config \
    libssl-dev \
    libuv1-dev \
    uuid-dev \
    zlib1g-dev \
    libjsoncpp-dev \
    ca-certificates \
 && rm -rf /var/lib/apt/lists/*
# Copy only src/include/cmake directories
COPY src/ src/
COPY include/ include/
COPY CMakeLists.txt CMakeLists.txt
COPY cmake/ cmake/


RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DATLASNET_DEV=OFF -DATLASNET_EXAMPLES=OFF -DATLASNET_TESTS=OFF
RUN cmake --build build --parallel --target AtlasNetController AtlasNetGateway AtlasNetCartographBackend AtlasNetDummy

FROM debian:trixie-slim AS runner
WORKDIR /atlasnet


FROM runner AS atlasnet-controller
COPY --from=builder /atlasnet/build/AtlasNetController /atlasnet/
ENTRYPOINT ["/atlasnet/AtlasNetController"]

FROM runner AS atlasnet-gateway
COPY --from=builder /atlasnet/build/AtlasNetGateway /atlasnet/
ENTRYPOINT ["/atlasnet/AtlasNetGateway"]

FROM runner AS atlasnet-dummy
COPY --from=builder /atlasnet/build/AtlasNetDummy /atlasnet/
ENTRYPOINT ["/atlasnet/AtlasNetDummy"]

FROM runner AS atlasnet-cartograph
#COPY --from=builder /atlasnet/build/AtlasNetCartographBackend /atlasnet/
#COPY web/ /atlasnet/frontend/
#WORKDIR /atlasnet/frontend
#RUN npm install
#RUN npm run build
#ENTRYPOINT ["/atlasnet/AtlasNetCartographBackend"]

