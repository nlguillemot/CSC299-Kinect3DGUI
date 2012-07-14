#include "irrfn.hpp"
#include "global.hpp"
#include <irrlicht/irrlicht.h>
#include <libfreenect/libfreenect.h>
#include <cstdio>

using namespace irr;

namespace ng
{

static IrrlichtDevice* device;

void initialize_irrlicht()
{
    device = createDevice(video::EDT_OPENGL, core::dimension2du(kSCREEN_WIDTH, kSCREEN_HEIGHT), 16, false, false, true, 0);
    device->setWindowCaption(L"CSC299-Kinect3DGUI");
}

// draws red green blue arrows to help know how the X Y Z coordinates work
static void draw_axis(irr::video::IVideoDriver* driver)
{
	using namespace irr;

	const float axisLength = 10.0f;
	const core::vector3df center(0.0f);
	const core::vector3df arrows[] = {
		center + core::vector3df(axisLength, 0.0f, 0.0f),
		center + core::vector3df(0.0f, axisLength, 0.0f),
		center + core::vector3df(0.0f, 0.0f, axisLength)
	};
	const video::SColor arrow_colors[] = {
		video::SColor(255, 255,   0,   0),
		video::SColor(255,   0, 255,   0),
		video::SColor(255,   0,   0, 255)
	};
	
	video::SMaterial material;
	material.Lighting = false;
	driver->setMaterial(material);
	driver->setTransform(video::ETS_WORLD, core::IdentityMatrix);

	driver->draw3DBox( core::aabbox3df(0.0f,0.0f,0.0f,10.0f,10.0f,10.0f) , video::SColor(255, 255, 255, 255) );
	for (unsigned i = 0; i < sizeof(arrows)/sizeof(*arrows); ++i)
	{
		driver->draw3DLine(center, arrows[i], arrow_colors[i % (sizeof(arrow_colors)/(sizeof(*arrow_colors)))]);
	}
}

// returns the average depth in 3D after a bandpass filter has been applied to it.
// coordinates are flipped horizontally and vertically
static core::vector3df average_depth_position(DepthBuffer* d, float lower_bound, float upper_bound)
{
    core::vector3df sumv;
    int count = 0;

    for (int y = 0; y < d->height; ++y)
    {
        for (int x = 0; x < d->width; ++x)
        {
            float value = d->get(x,y);
            if (value >= lower_bound && value <= upper_bound)
            {
                sumv += core::vector3df(d->width - x, d->height - y, value);
                ++count;
            }
        }
    }

    if (count != 0)
    {
        sumv /= count;
    }

    return sumv;
}

static core::vector3df nearest_depth_position(DepthBuffer* d)
{
    core::vector3df nearest(0,0,FREENECT_DEPTH_RAW_NO_VALUE);

    for (int y = 0; y < d->height; ++y)
    {
        for (int x = 0; x < d->width; ++x)
        {
            float value = d->get(x,y);
            if (value < nearest.Z)
            {
                nearest = core::vector3df(d->width - x, d->height - y,value);
            }
        }
    }

    return nearest;
}

static void draw_depth_buffer(video::IVideoDriver* drv, DepthBuffer* d, const video::SColor& c, float lower_bound, float upper_bound)
{
    for (int y = 0; y < d->height; ++y)
    {
        for (int x = 0; x < d->width; ++x)
        {
            float value = d->get(x,y);
            if (value >= lower_bound && value <= upper_bound)
            {
                drv->drawPixel(d->width - x, y, c);
            }
        }
    }
}

template<class T, class K>
static void set_aabbox_centre(core::aabbox3d<T>& box, const core::vector3d<K>& centre)
{
    core::vector3d<T> extents = box.getExtent();
    box.MinEdge.set(centre - (extents/2));
    box.MaxEdge.set(centre + (extents/2));
}

void irr_main()
{
    video::IVideoDriver* driver = device->getVideoDriver();
    scene::ISceneManager* smgr = device->getSceneManager();
    gui::IGUIEnvironment* guienv = device->getGUIEnvironment();

    // represents location of hand in 3D space
    core::aabbox3df hand_box(-10, -10, -10, 10, 10, 10);
    video::SColor hand_color(255, 255, 0, 0);

    // hand is always tweened toward this point
    core::vector3df hand_target_pos(0,0,FREENECT_DEPTH_RAW_NO_VALUE);

    // world units per second
    float hand_tween_speed = 12.0f;

    // map 3D space to freenect depth space
    scene::ICameraSceneNode* cam = smgr->addCameraSceneNode(0, core::vector3df(kSCREEN_WIDTH/2, kSCREEN_HEIGHT/2,0), core::vector3df(kSCREEN_WIDTH/2, kSCREEN_HEIGHT/2, 1));
    core::matrix4 proj;
    proj.buildProjectionMatrixOrthoLH(kSCREEN_WIDTH, kSCREEN_HEIGHT, 0, FREENECT_DEPTH_RAW_MAX_VALUE);
    cam->setProjectionMatrix(proj, true);

    // bounds for depth filtering
    float lower_bound = 500, upper_bound = 800;

    // time stuff
    u32 now = 0, then = 0;

    while (!g_die)
    {
        if (device->run())
        {
            // Grab all available depth buffers.
            std::vector<DepthBuffer*> bufs;
            g_depth_buffer_queue.lock();
            while (!g_depth_buffer_queue.empty())
            {
                bufs.push_back(g_depth_buffer_queue.pop());
            }
            g_depth_buffer_queue.unlock();

            if (!bufs.empty())
            {
                // extract position of hand from most recent depth buffer and lead the cursor to it
                core::vector3df next_hand_target_pos = nearest_depth_position(bufs.back());

                if (next_hand_target_pos.Z != FREENECT_DEPTH_RAW_NO_VALUE)
                {
                    // if the target position has not yet been initialized, just snap to it.
                    if (hand_target_pos.Z == FREENECT_DEPTH_RAW_NO_VALUE)
                    {
                        set_aabbox_centre(hand_box, next_hand_target_pos);
                    }

                    hand_target_pos = next_hand_target_pos;
                    hand_color.set(255, 0, 255, 0);
                }
                else
                {
                    hand_color.set(255, 255, 0, 0);
                }
            }

            // update delta time
            now = device->getTimer()->getTime();
            u32 dt = now - then;
            then = now;

            // update hand position
            core::vector3df hand_distance_to_target = hand_target_pos - hand_box.getCenter();

            core::vector3df delta_hand_position = hand_distance_to_target;
            delta_hand_position.normalize();
            delta_hand_position *= hand_tween_speed;
            delta_hand_position * (dt/1000.0);

            // prevent jittery movement near target position
            if (delta_hand_position.getLengthSQ() < hand_distance_to_target.getLengthSQ())
            {
                set_aabbox_centre(hand_box, hand_box.getCenter() + delta_hand_position);
            }
            else
            {
                set_aabbox_centre(hand_box, hand_target_pos);
            }


            // Render scene
            if (device->isWindowActive())
            {
                driver->beginScene(true,true,video::SColor(255,100,149,237));

                smgr->drawAll();
                guienv->drawAll();

                draw_axis(driver);

                // draw hand
                driver->draw3DBox(hand_box, hand_color);

                if (!bufs.empty())
                {
                    draw_depth_buffer(driver, bufs.back(), video::SColor(255,0,0,255), lower_bound, upper_bound);
                }

                driver->endScene();
            }
            else
            {
                device->yield();
            }

            // Throw all collected buffers back in the pool
            g_depth_buffer_pool.lock();
            for (DepthBuffer* b : bufs)
            {
                g_depth_buffer_pool.deallocate(b);
            }
            g_depth_buffer_pool.unlock();
        }
        else
        {
            g_die = true;
        }
    }
}

}
