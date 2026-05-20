#include "AtlasNetAgent.hpp"


int main(int argc, char** argv) {
    AtlasNet::AtlasNetAgent agent;
    agent.Init();
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}