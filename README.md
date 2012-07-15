CSC299-Kinect3DGUI
==================

This is a proof of concept to see how well a 3D GUI can be interfaced with using the kinect.

There already exist projects done to play with GUIs using the kinect, otherwise known as the "Minority Report" style of interaction. An example of such a project can be seen here: http://www.youtube.com/watch?v=tlLschoMhuE
While this demo by Willow Garage is totally awesome, the graphical elements with which the user is interacting are in 2D.
This project's goal is to see if the functionality can be extended to interacting with 3D window elements.

Tools used:

+ Freenect for interfacing with the Kinect - https://github.com/OpenKinect/libfreenect
+ Irrlicht for 3D rendering - https://github.com/zaki/irrlicht
+ IrrKlang for sound - http://www.ambiera.com/irrklang/
+ Scons for building - http://www.scons.org/
+ gcc for compiling as C++11 - http://gcc.gnu.org/