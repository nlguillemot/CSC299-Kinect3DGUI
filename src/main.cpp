#include <irrlicht/irrlicht.h>
#include <thread>
#include "kinectfn.hpp"
#include "irrfn.hpp"

int main()
{
    std::thread freenect_thread(ng::freenect_main);
    ng::irr_main();
    freenect_thread.join();
}
