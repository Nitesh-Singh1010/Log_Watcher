#pragma once
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include "circular_buffer.hpp"
 
class LogWatcher
{
public:
    explicit LogWatcher(const std::string &filename);
    ~LogWatcher();
 
    void start();
    void stop();
    std::vector<std::string> getLastLines() const;
    void setCallback(std::function<void(const std::string &)> callback);
 
private:
    void watchLoop();
    void readLastLines();
 
    std::string filename_;
    std::atomic<bool> running_;
    std::thread watch_thread_;
    CircularBuffer<std::string> last_lines_;
    std::function<void(const std::string &)> callback_;
    long long last_position_;
};