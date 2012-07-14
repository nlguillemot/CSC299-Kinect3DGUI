#ifndef NG_KINECTFN_HPP_INCLUDED
#define NG_KINECTFN_HPP_INCLUDED

#include <libfreenect/libfreenect.h>

namespace ng
{

void freenect_main();

int initialize_freenect(freenect_context** ctx, freenect_device** dev);

}

#endif // NG_KINECTFN_HPP_INCLUDED
