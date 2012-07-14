#include "global.hpp"
#include "SynchronizedQueue.hpp"

namespace ng
{

bool g_die = false;

SynchronizedQueue<DepthBuffer*> g_depth_buffer_queue;

SynchronizedPoolAllocator<DepthBuffer,10> g_depth_buffer_pool;

}
