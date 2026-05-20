#pragma once

#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

class DockerStackTest : public ::testing::Test
{
protected:
    // Change these in your derived test if needed.
    std::string ComposeFile = std::string(TEST_COMPOSE_FILE_PATH) + "/docker-compose.yml";
    std::string StackName   = "atlasnet_test_stack";

    void SetUp() override
    {
        AssertFileExists(ComposeFile);
        EnsureDockerExists();
        EnsureSwarmActive();
        DeployStack(ComposeFile, StackName);

        // Optional small delay so services have time to start.
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    void TearDown() override
    {
        RemoveStack(StackName);

        // Optional wait so stack actually disappears before next test.
        WaitForStackRemoval(StackName, std::chrono::seconds(20));
    }
    
    std::string GetControllerLogs() const
    {
        std::ostringstream cmd;
        cmd << "docker service logs " << StackName << "_Atlasnet-Controller --no-task-ids --raw";
        std::array<char, 128> buffer;
        std::string result;

        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.str().c_str(), "r"), pclose);
        if (!pipe)
        {
            throw std::runtime_error("Failed to get controller logs");
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        {
            result += buffer.data();
        }
        return result;
    }
private:
    static void AssertFileExists(const std::string& path)
    {
        
        ASSERT_TRUE(std::filesystem::exists( path))
            << "Compose file does not exist: " << path;
    }

    static int RunCommand(const std::string& command)
    {
        return std::system(command.c_str());
    }

    static std::string Quote(const std::string& s)
    {
        return "\"" + s + "\"";
    }

    static void EnsureDockerExists()
    {
        const int rc = RunCommand("docker --version > /dev/null 2>&1");
        ASSERT_EQ(rc, 0) << "Docker does not appear to be installed or accessible.";
    }

    static void EnsureSwarmActive()
    {
        const int rc = RunCommand(
            "docker info --format '{{.Swarm.LocalNodeState}}' | grep -q '^active$'");
        ASSERT_EQ(rc, 0)
            << "Docker Swarm is not active. Run: docker swarm init";
    }

    static void DeployStack(const std::string& composeFile, const std::string& stackName)
    {
        std::ostringstream cmd;
        cmd << "docker stack deploy -c "
            << Quote(composeFile)
            << " "
            << stackName
            << " > /dev/null";

        const int rc = RunCommand(cmd.str());
        ASSERT_EQ(rc, 0)
            << "Failed to deploy stack '" << stackName
            << "' using compose file '" << composeFile << "'";
    }

    static void RemoveStack(const std::string& stackName)
    {
        std::ostringstream cmd;
        cmd << "docker stack rm " << stackName << " > /dev/null 2>&1";
        RunCommand(cmd.str());
    }

    static bool StackStillExists(const std::string& stackName)
    {
        std::ostringstream cmd;
        cmd << "docker stack ls --format '{{.Name}}' | grep -q '^"
            << stackName
            << "$'";
        return RunCommand(cmd.str()) == 0;
    }

    static void WaitForStackRemoval(
        const std::string& stackName,
        std::chrono::seconds timeout)
    {
        const auto start = std::chrono::steady_clock::now();

        while (true)
        {
            if (!StackStillExists(stackName))
                return;

            if (std::chrono::steady_clock::now() - start > timeout)
            {
                ADD_FAILURE() << "Timed out waiting for stack removal: " << stackName;
                return;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
};