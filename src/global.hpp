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
    uint16_t buf[kSCREEN_WIDTH * kSCREEN_HEIGHT * 3];
};

// true if a kill signal has been received by the application
extern bool g_die;

extern SynchronizedQueue<DepthBuffer*> g_depth_buffer_queue;

extern SynchronizedPoolAllocator<DepthBuffer,10> g_depth_buffer_pool;

}

#endif // NG_GLOBAL_HPP_INCLUDED
