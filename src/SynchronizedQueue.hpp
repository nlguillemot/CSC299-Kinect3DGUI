#ifndef NG_SYNCHRONIZED_QUEUE_HPP_INCLUDED
#define NG_SYNCHRONIZED_QUEUE_HPP_INCLUDED

#include <thread>
#include <queue>

namespace ng
{

// wrapper for std::queue<T>
// must manually call lock and unlock before using.
template<class T, class Container = std::deque<T>>
class SynchronizedQueue
{
public:
    void push(const T& t)
    {
        m_queue.push(t);
    }

    T pop()
    {
        T front = m_queue.front();
        m_queue.pop();
        return front;
    }

    T peek() const
    {
        return m_queue.front();
    }

    size_t size() const
    {
        return m_queue.size();
    }

    bool empty() const
    {
        return m_queue.empty();
    }

    void lock()
    {
        m_mutex.lock();
    }

    void unlock()
    {
        m_mutex.unlock();
    }

private:
    mutable std::mutex m_mutex;
    std::queue<T,Container> m_queue;
};

}

#endif // NG_SYNCHRONIZED_QUEUE_HPP_INCLUDED
