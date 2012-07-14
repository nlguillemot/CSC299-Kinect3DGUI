#include "irrfn.hpp"
#include "global.hpp"
#include <irrlicht/irrlicht.h>

using namespace irr;

namespace ng
{

static IrrlichtDevice* device;

void initialize_irrlicht()
{
    device = createDevice(video::EDT_OPENGL, core::dimension2du(kSCREEN_WIDTH,kSCREEN_HEIGHT), 16, false, false, true, 0);
    device->setWindowCaption(L"CSC299-Kinect3DGUI");
}

void irr_main()
{
    video::IVideoDriver* driver = device->getVideoDriver();
    scene::ISceneManager* smgr = device->getSceneManager();
    gui::IGUIEnvironment* guienv = device->getGUIEnvironment();

    while (!g_die)
    {
        if (device->run())
        {
            // Grab all available buffers.
            std::vector<DepthBuffer*> bufs;
            g_depth_buffer_queue.lock();
            while (!g_depth_buffer_queue.empty())
            {
                bufs.push_back(g_depth_buffer_queue.pop());
            }
            g_depth_buffer_queue.unlock();

            // TODO: Process all buffers

            // Throw all collected buffers back in the pool
            g_depth_buffer_pool.lock();
            for (DepthBuffer* b : bufs)
            {
                g_depth_buffer_pool.deallocate(b);
            }
            g_depth_buffer_pool.unlock();

            // Render irrlicht things
            if (device->isWindowActive())
            {
                driver->beginScene(true,true,video::SColor(255,100,149,237));
                smgr->drawAll();
                guienv->drawAll();
                driver->endScene();
            }
            else
            {
                device->yield();
            }
        }
        else
        {
            g_die = true;
        }
    }
}

}
