#include "kinectfn.hpp"
#include "global.hpp"
#include <libfreenect/libfreenect.h>
#include <cstdio>

namespace ng
{

static freenect_context* freenect_ctx;
static freenect_device* freenect_dev;

static void depth_callback(freenect_device* dev, void* v_depth, uint32_t timestamp);

int initialize_freenect()
{
    // initialize freenect library
    if (freenect_init(&freenect_ctx, 0) < 0)
    {
        return 1;
    }

    // enable debug logging
    freenect_set_log_level(freenect_ctx, FREENECT_LOG_DEBUG);

    // choose to only enable the camera
    freenect_select_subdevices(freenect_ctx, (freenect_device_flags)(FREENECT_DEVICE_CAMERA));

    // make sure we can detect at least 1 device
    if (freenect_num_devices(freenect_ctx) < 0)
    {
        freenect_shutdown(freenect_ctx);
        return 1;
    }

    if (freenect_open_device(freenect_ctx, &freenect_dev, 0) < 0)
    {
        freenect_shutdown(freenect_ctx);
        return 1;
    }

    return 0;
}

void freenect_main()
{
    // hook up depth configuration
    freenect_set_depth_callback(freenect_dev, depth_callback);
    freenect_set_depth_mode(freenect_dev, freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_11BIT));

    // start generating depth callbacks
    freenect_start_depth(freenect_dev);

    // main loop for gathering freenect events
    while (!g_die && freenect_process_events(freenect_ctx) >= 0);

    // disable depth camera
    freenect_stop_depth(freenect_dev);

    // close device
    freenect_close_device(freenect_dev);

    freenect_shutdown(freenect_ctx);
}

void depth_callback(freenect_device* dev, void* v_depth, uint32_t timestamp)
{
    g_depth_buffer_queue.lock();
    g_depth_buffer_queue.push(static_cast<DepthBuffer*>(v_depth));
    g_depth_buffer_queue.unlock();

    DepthBuffer* next_buffer;

    do
    {
        g_depth_buffer_pool.lock();
        next_buffer = g_depth_buffer_pool.allocate();
        g_depth_buffer_pool.unlock();
    }
    while (!next_buffer && !g_die);

    freenect_set_depth_buffer(dev, next_buffer);
}

}
