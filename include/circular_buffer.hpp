#pragma once
#include <vector>
#include <string>
#include <mutex>
 
template <typename T>
class CircularBuffer
{
public:
    explicit CircularBuffer(size_t size) : max_size_(size), current_pos_(0)
    {
        buffer_.reserve(size);
    }
 
    void push(const T &item)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (buffer_.size() < max_size_)
        {
            buffer_.push_back(item);
        }
        else
        {
            buffer_[current_pos_] = item;
        }
        current_pos_ = (current_pos_ + 1) % max_size_;
    }
 
    std::vector<T> getAll() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<T> result;
        if (buffer_.size() < max_size_)
        {
            return buffer_;
        }
 
        result.reserve(max_size_);
        for (size_t i = 0; i < max_size_; ++i)
        {
            size_t pos = (current_pos_ + i) % max_size_;
            result.push_back(buffer_[pos]);
        }
        return result;
    }
 
private:
    std::vector<T> buffer_;
    const size_t max_size_;
    size_t current_pos_;
    mutable std::mutex mutex_;
};