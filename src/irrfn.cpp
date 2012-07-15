#include "irrfn.hpp"
#include "global.hpp"
#include <irrlicht/irrlicht.h>
#include <irrklang/irrKlang.h>
#include <libfreenect/libfreenect.h>
#include <cstdio>

using namespace irr;
using namespace irrklang;

namespace ng
{

static IrrlichtDevice* device;
static ISoundEngine* sengine;

int initialize_irrlicht()
{
    device = createDevice(video::EDT_OPENGL, core::dimension2du(kSCREEN_WIDTH, kSCREEN_HEIGHT), 16, false, false, true, 0);
    if (!device)
    {
        return 1;
    }

    device->setWindowCaption(L"CSC299-Kinect3DGUI");

    sengine = createIrrKlangDevice();
    if (!sengine)
    {
        return 1;
    }

    return 0;
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
                nearest = core::vector3df(d->width - x, d->height - y, value);
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

scene::IMeshSceneNode* add_rectangular_prism_mesh(scene::ISceneManager* smgr, video::IVideoDriver* driver, scene::ISceneNode* parent, const core::vector3df& dim, const char* color)
{
    using video::S3DVertex;

/*
   /0--------/3
  / |       / |
 /  |      /  |
1---------2   |
|  /4- - -|- -7
| /       |  /
|/        | /
5---------6/
*/
    // TODO: Fix u,v coordinates
    video::SColor white(255,255,255,255);
    const S3DVertex verts[] = {
        S3DVertex(0, 0, 0, -1, 1, 1, white, 0, 0),
        S3DVertex(0, 0, dim.Z, -1, 1, -1, white, 0, 0),
        S3DVertex(dim.X, 0, dim.Z, 1, 1, -1, white, 0, 0),
        S3DVertex(dim.X, 0, 0, 1, 1, 1, white, 0, 0),
        S3DVertex(0, -dim.Y, 0, 1, -1, 1, white, 0, 0),
        S3DVertex(0, -dim.Y, dim.Z, -1, -1, -1, white, 0, 0),
        S3DVertex(dim.X, -dim.Y, dim.Z, 1, -1, -1, white, 0, 0),
        S3DVertex(dim.X, -dim.Y, 0, 1, -1, 1, white, 0, 0)
    };

    const u16 indices[] = {
        0, 1, 2, // top
        0, 2, 3, // top
        4, 0, 3, // back
        4, 3, 7, // back
        3, 2, 6, // right
        3, 6, 7, // right
        4, 5, 1, // left
        4, 1, 0, // left
        1, 5, 6, // front
        1, 6, 2, // front
        5, 4, 7, // bottom
        5, 7, 6, // bottom
    };

    scene::IMeshBuffer* b = new scene::SMeshBuffer();
    b->append(verts, sizeof(verts)/sizeof(*verts), indices, sizeof(indices)/sizeof(*indices));

    scene::SMesh* m = new scene::SMesh();
    m->addMeshBuffer(b);
    m->setBoundingBox(b->getBoundingBox());

    scene::IMeshSceneNode* created_node = smgr->addMeshSceneNode(m, parent);
    created_node->setMaterialTexture(0, driver->getTexture(color));

    m->drop();
    b->drop();

    return created_node;
}

struct DialogBox
{
    scene::ISceneNode* root;
    scene::IMeshSceneNode* panel;
    scene::IMeshSceneNode* exit_button;
    scene::IMeshSceneNode* menu_bar;
    scene::IMeshSceneNode* main_button;

    bool intersect_main;
};

core::aabbox3df get_node_bounds_recursively(scene::ISceneNode* node)
{
    core::aabbox3df box = node->getTransformedBoundingBox();
    core::list<scene::ISceneNode*> children = node->getChildren();
    for (auto it = children.begin(); it != children.end(); ++it)
    {
        box.addInternalBox(get_node_bounds_recursively(*it));
    }
    return box;
}

void hook_up_dialog_box(DialogBox* d, scene::ISceneManager* smgr, video::IVideoDriver* driver)
{
    const float width = 200;
    const float height = 200;
    const float paneldepth = 20;
    const float buttondepth = 50;

    d->root = smgr->addEmptySceneNode();

    d->panel = add_rectangular_prism_mesh(smgr, driver, d->root, core::vector3df(width, height, paneldepth), "white");

    d->exit_button = add_rectangular_prism_mesh(smgr, driver, d->root, core::vector3df(width/5, height/5, buttondepth), "red");
    d->exit_button->setPosition(core::vector3df(width*4/5, 0, -paneldepth));

    d->menu_bar = add_rectangular_prism_mesh(smgr, driver, d->root, core::vector3df(width*4/5, height/5, buttondepth), "blue");
    d->menu_bar->setPosition(core::vector3df(0,0, -paneldepth));

    d->main_button = add_rectangular_prism_mesh(smgr, driver, d->root, core::vector3df(width*3/5, height/5, buttondepth), "grey");
    d->main_button->setPosition(core::vector3df(width/5, -height*4/5, -paneldepth));
    d->intersect_main = false;
}

template<class T>
void print_vector3(const core::vector3d<T>& v)
{
    printf("{ %f, %f, %f }", v.X, v.Y, v.Z);
}

template<class T>
void print_aabbox(const core::aabbox3d<T>& bbox)
{
    printf("{ MinEdge: ");
    print_vector3(bbox.MinEdge);
    printf(" MaxEdge: ");
    print_vector3(bbox.MaxEdge);
    printf(" }");
}

void print_color(const video::SColor& c)
{
    printf("ARGBu: { %u, %u, %u, %u }", c.getAlpha(), c.getRed(), c.getGreen(), c.getBlue());
}

void print_color(const video::SColorf& c)
{
    printf("RGBAf: { %f, %f, %f, %f }", c.getRed(), c.getGreen(), c.getBlue(), c.getAlpha());
}

// seems like there is no simpler way to change colors of things at runtime...
void add_colors_to_irrlicht(video::IVideoDriver* driver)
{
    using video::SColor;

    const char* color_names[] = {
        "red",
        "green",
        "blue",
        "white",
        "black",
        "grey",
        "light_blue",
    };

    SColor colors[] = {
        SColor(255,255,0,0),
        SColor(255,0,255,0),
        SColor(255,0,0,255),
        SColor(255,255,255,255),
        SColor(255,0,0,0),
        SColor(255,100,100,100),
        SColor(255,173, 216, 230),
    };

    for (size_t i = 0; i < sizeof(colors)/sizeof(*colors); ++i)
    {
        video::IImage* c = driver->createImage(video::ECF_A8R8G8B8, core::dimension2du(128,128));
        c->fill(colors[i]);
        driver->addTexture(color_names[i],c);
    }
}

void irr_main()
{
    video::IVideoDriver* driver = device->getVideoDriver();
    scene::ISceneManager* smgr = device->getSceneManager();
    gui::IGUIEnvironment* guienv = device->getGUIEnvironment();

    sengine->play2D("data/plopp.wav");

    add_colors_to_irrlicht(driver);

    // represents location of hand in 3D space
    core::aabbox3df hand_box(-10, -10, -10, 10, 10, 10);
    video::SColor hand_color(255, 255, 0, 0);

    // hand is always tweened toward this point
    core::vector3df hand_target_pos(0,0,FREENECT_DEPTH_RAW_NO_VALUE);

    // world units per second
    float hand_tween_speed = 12.0f;

    scene::ICameraSceneNode* cam;
#if 1
    // map 3D space to freenect depth space
    cam = smgr->addCameraSceneNode(0, core::vector3df(kSCREEN_WIDTH/2, kSCREEN_HEIGHT/2,0), core::vector3df(kSCREEN_WIDTH/2, kSCREEN_HEIGHT/2, 1));
#else
    // FPS camera for easier debugging of graphics
    cam = smgr->addCameraSceneNodeFPS();
#endif

    // bounds for depth filtering
    float lower_bound = 500, upper_bound = 1300;

    core::matrix4 proj;
    // proj.buildProjectionMatrixOrthoLH(kSCREEN_WIDTH, kSCREEN_HEIGHT, 0, FREENECT_DEPTH_RAW_MAX_VALUE);
    proj.buildProjectionMatrixPerspectiveLH(kSCREEN_WIDTH, kSCREEN_HEIGHT, lower_bound, upper_bound);
    cam->setProjectionMatrix(proj, true);

    // add some lighting so that everything isn't just black
    scene::ILightSceneNode* hand_light = smgr->addLightSceneNode(0, hand_box.getCenter(), video::SColorf(1.0f,1.0f,1.0f), 100);

    // set ambient color to grey
    smgr->setAmbientLight(video::SColorf(0.5, 0.5, 0.5));

    // initialize dialog box
    DialogBox dialog_box;
    hook_up_dialog_box(&dialog_box, smgr, driver);
    core::vector3df dialog_extents = get_node_bounds_recursively(dialog_box.root).getExtent();
    dialog_box.root->setPosition(core::vector3df(kSCREEN_WIDTH/2 - dialog_extents.X/2, kSCREEN_HEIGHT/2 + dialog_extents.Y/2, upper_bound/2));

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
                // when things are near the kinect, they are far away from us. need to sorta invert it to fix that.
                next_hand_target_pos.Z = upper_bound - next_hand_target_pos.Z;

                if (next_hand_target_pos.Z != FREENECT_DEPTH_RAW_NO_VALUE)
                {
                    // if the target position has not yet been initialized, just snap to it.
                    if (hand_target_pos.Z == FREENECT_DEPTH_RAW_NO_VALUE)
                    {
                        set_aabbox_centre(hand_box, next_hand_target_pos);
                    }

                    hand_target_pos = next_hand_target_pos;
/*
                    printf("new target: ");
                    print_vector3(hand_target_pos);
                    printf("\n");
*/

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

            // move light with hand
            hand_light->setPosition(hand_box.getCenter());

            // check for collisions between the hand and GUI elements
            if (dialog_box.main_button->getTransformedBoundingBox().isPointTotalInside(hand_box.getCenter()))
            {
                if (!dialog_box.intersect_main && delta_hand_position.Z > 5)
                {
                    dialog_box.main_button->setMaterialTexture(0, driver->getTexture("light_blue"));
                    dialog_box.intersect_main = true;
                    sengine->play2D("data/blip.wav");
                }
            }
            else if (!dialog_box.main_button->getTransformedBoundingBox().intersectsWithBox(hand_box))
            {
                dialog_box.main_button->setMaterialTexture(0, driver->getTexture("grey"));
                dialog_box.intersect_main = false;
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

/*
                if (!bufs.empty())
                {
                    draw_depth_buffer(driver, bufs.back(), video::SColor(255,0,0,255), lower_bound, upper_bound);
                }
*/

                driver->endScene();
            }
            else
            {
                device->yield();
            }

            // Throw all collected buffers back in the pool
            g_depth_buffer_pool.lock();
            for (size_t i = 0; i < bufs.size(); ++i)
            {
                g_depth_buffer_pool.deallocate(bufs[i]);
            }
            g_depth_buffer_pool.unlock();
            bufs.clear();
        }
        else
        {
            g_die = true;
        }
    }

    device->drop();
    sengine->drop();
}

}
