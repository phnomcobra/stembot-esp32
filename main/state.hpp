#pragma once

// Process-wide Config and AgentClient instances, shared by configure.cpp,
// crypto.cpp, processor.cpp, and main.cpp. Defined here as C++17 inline
// variables so every translation unit that includes this header refers to
// the exact same objects, without each file re-declaring its own static
// alias.

#include "agent.hpp"
#include "config.hpp"

inline Config config;
inline AgentClient client;
