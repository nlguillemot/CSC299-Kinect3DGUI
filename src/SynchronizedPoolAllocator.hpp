#ifndef NG_SYNCHRONIZED_POOL_ALLOCATOR_HPP_INCLUDED
#define NG_SYNCHRONIZED_POOL_ALLOCATOR_HPP_INCLUDED

#include <thread>

// not automatically synchronized, must lock and unlock.
template<class T, size_t N>
class SynchronizedPoolAllocator
{

static_assert (sizeof(T) >= sizeof(T*), "Pool elements must be at least the size of a pointer.");
static_assert (N > 0, "Pool cannot be declared empty.");

public:
    SynchronizedPoolAllocator():
    m_head(m_pool)
    {
        for (int i = 0; i < int(N) - 1; ++i)
        {
            T** block = reinterpret_cast<T**>(&m_pool[i]);
            *block = &m_pool[i+1];
        }

        T** tail = reinterpret_cast<T**>(&m_pool[N-1]);
        *tail = 0;
    }

    T* allocate()
    {
        if (m_head == 0) return 0;

        T* oldhead = m_head;
        m_head = *reinterpret_cast<T**>(m_head);
        return oldhead;
    }

    void deallocate(T* elem)
    {
        *reinterpret_cast<T**>(elem) = m_head;
        m_head = elem;
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
    T* m_head;
    T m_pool[N];
    mutable std::mutex m_mutex;
};

#endif // NG_SYNCHRONIZED_POOL_ALLOCATOR_HPP_INCLUDED
