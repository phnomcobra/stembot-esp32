#pragma once

#include "agent.hpp"
#include "config.hpp"

inline Config& shared_config()
{
    static Config instance;
    return instance;
}

inline AgentClient& shared_agent_client()
{
    static AgentClient instance;
    return instance;
}
