#pragma once
#include "PlatformIncludes.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <future>
#include <sstream>
#include <thread>
#include <mutex>
// Production uses spdlog; suppress expected disconnect messages in stress tests.
template<typename... Args> void test_log(const char*, const Args&...) {}
#define LOG_SAVE(...) test_log(__VA_ARGS__)
