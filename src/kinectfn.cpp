#include "kinectfn.hpp"
#include "global.hpp"
#include <libfreenect/libfreenect.h>

namespace ng
{

static void depth_callback(freenect_device* dev, void* v_depth, uint32_t timestamp);

void freenect_main()
{
    freenect_context* freenect_ctx;
    freenect_device* freenect_dev;

    if (initialize_freenect(&freenect_ctx, &freenect_dev))
    {
        return;
    }

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
}

int initialize_freenect(freenect_context** ctx, freenect_device** dev)
{
    // initialize freenect library
    if (freenect_init(ctx, 0))
    {
        return 1;
    }

    // enable debug logging
    freenect_set_log_level(*ctx, FREENECT_LOG_DEBUG);

    // choose to only enable the camera
    freenect_select_subdevices(*ctx, (freenect_device_flags)(FREENECT_DEVICE_CAMERA));

    // make sure we can detect at least 1 device
    if (freenect_num_devices(*ctx) < 1)
    {
        return 1;
    }

    if (freenect_open_device(*ctx, dev, 0) < 0)
    {
        freenect_shutdown(*ctx);
        return 1;
    }

    return 0;
}

void depth_callback(freenect_device* dev, void* v_depth, uint32_t timestamp)
{
}

}
