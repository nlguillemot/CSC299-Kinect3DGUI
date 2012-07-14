#ifndef NG_GLOBAL_HPP_INCLUDED
#define NG_GLOBAL_HPP_INCLUDED

#include "SynchronizedQueue.hpp"
#include "SynchronizedPoolAllocator.hpp"

namespace ng
{

static const int kSCREEN_WIDTH = 640;
static const int kSCREEN_HEIGHT = 480;

struct DepthBuffer
{
    static const int width = kSCREEN_WIDTH;
    static const int height = kSCREEN_HEIGHT;

    /* must be the only variable in this struct,
     * because reinterpret_cast<uint16_t*>(some_depth_buffer_ptr)
     * must be possible.
     */
    uint16_t buf[width * height];

    uint16_t get(int x, int y)
    {
        return buf[y * width + x];
    }
};

// true if a kill signal has been received by the application
extern bool g_die;

extern SynchronizedQueue<DepthBuffer*> g_depth_buffer_queue;

extern SynchronizedPoolAllocator<DepthBuffer,10> g_depth_buffer_pool;

}

#endif // NG_GLOBAL_HPP_INCLUDED
