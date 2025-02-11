#include "log_watcher.hpp"
#include <fstream>
#include <thread>
#include <chrono>

LogWatcher::LogWatcher(const std::string &filename)
    : filename_(filename), running_(false), last_lines_(10), last_position_(0)
{
    readLastLines();
}

LogWatcher::~LogWatcher()
{
    stop();
}

void LogWatcher::start()
{
    running_ = true;
    watch_thread_ = std::thread(&LogWatcher::watchLoop, this);
}

void LogWatcher::stop()
{
    running_ = false;
    if (watch_thread_.joinable())
    {
        watch_thread_.join();
    }
}

void LogWatcher::setCallback(std::function<void(const std::string &)> callback)
{
    callback_ = std::move(callback);
}

std::vector<std::string> LogWatcher::getLastLines() const
{
    return last_lines_.getAll();
}

void LogWatcher::readLastLines()
{
    std::ifstream file(filename_, std::ios::ate);
    if (!file)
        return;

    long long file_size = file.tellg();
    long long pos = file_size;
    int lines_count = 0;
    std::vector<std::string> lines;

    std::vector<char> buffer(4096);
    while (pos > 0 && lines_count < 10)
    {
        long long read_size = std::min(pos, static_cast<long long>(buffer.size()));
        pos -= read_size;

        file.seekg(pos);
        file.read(buffer.data(), read_size);

        for (long long i = read_size - 1; i >= 0; --i)
        {
            if (buffer[i] == '\n')
            {
                ++lines_count;
                if (lines_count >= 10)
                    break;
            }
        }
    }

    // Read the last 10 lines
    file.seekg(pos);
    std::string line;
    while (std::getline(file, line))
    {
        last_lines_.push(line);
    }

    last_position_ = file_size;
}

void LogWatcher::watchLoop()
{
    while (running_)
    {
        std::ifstream file(filename_, std::ios::ate);
        if (!file)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        long long current_size = file.tellg();
        if (current_size > last_position_)
        {
            file.seekg(last_position_);
            std::string line;
            while (std::getline(file, line))
            {
                last_lines_.push(line);
                if (callback_)
                {
                    callback_(line);
                }
            }
            last_position_ = current_size;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}