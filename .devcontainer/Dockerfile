FROM ubuntu:26.04

RUN apt update && apt install -y build-essential libssl-dev libuv1-dev libjsoncpp-dev clang clangd cmake uuid-dev doxygen gdb
# Install X11 development libraries
RUN apt install -y libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev mesa-utils
# Install Python development libraries
RUN apt install -y  python3-venv 
# Install Drogon dependencies
RUN apt install -y zlib1g-dev
