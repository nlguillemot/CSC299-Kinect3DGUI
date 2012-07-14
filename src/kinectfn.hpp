#ifndef NG_KINECTFN_HPP_INCLUDED
#define NG_KINECTFN_HPP_INCLUDED

#include <libfreenect/libfreenect.h>

namespace ng
{

int initialize_freenect();

void freenect_main();

}

#endif // NG_KINECTFN_HPP_INCLUDED
