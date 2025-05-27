#ifndef BUFFERQUEUE_HPP
#define BUFFERQUEUE_HPP

#include <queue>
#include <mutex>
#include <condition_variable>

class BufferQueue {
public:
    BufferQueue();
    ~BufferQueue();

private:
    std::queue<std::string> queue; // Queue to hold the buffers
    std::mutex queueMutex;          // Mutex for thread-safe access to the queue
    std::condition_variable queueCondVar; // Condition variable for signaling when buffers are available
};

#endif