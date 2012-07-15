#include <irrlicht/irrlicht.h>
#include <thread>
#include "kinectfn.hpp"
#include "irrfn.hpp"

int main()
{
    /* the order in which things are initialized here is actually
     * very important to be as is. It's very easy to cause inexplicable
     * crashes if things are done out of order. Therefore, I suggest
     * not touching this stuff.
     */
    if (ng::initialize_irrlicht())
    {
        return 1;
    }
    if (ng::initialize_freenect())
    {
        return 1;
    }

    std::thread freenect_thread(ng::freenect_main);
    ng::irr_main();
    freenect_thread.join();
}
