# A Toy Ray-Tracing Program


This program was mainly a way to stretch my muscles with OpenGL and have some fun with compute
shaders. Modern OpenGL has placed more emphasis on using shaders to run graphics code on the GPU
rather than preprocessing most of it on the CPU. Since GPUs have better linear algebra processing
and parallel work groups, slow algorithms such as ray-tracing are greatly sped up on GPUs. Shaders
also have the added convenience of not needing to be precompiled with your program, but rather can
be compiled on start up, allowing developers to adjust shader code without having to completely
recompile. The use of compute shaders was inspired by this [lwjgl blog](https://blog.lwjgl.org/ray-tracing-with-opengl-compute-shaders-part-i/).


## Dependencies


The major dependencies are:
 * SDL2
 * OpenGL
 * GLEW

SDL is only necessary for handling windows and events. The program is also dependent on MVL,
which is a linear algebra I rolled myself. Other linear algebra libraries, like GLM, can
easily be used to replace it.


## Build


Starting at the parent directory, create a build folder and enter it.

```bash
mkdir build && cd build
```

Run cmake on the parent directory, then make the program.

```bash
cmake ..
make
```

Run the program.

```bash
./ray_tracer
```


## Ray Tracing With Compute Shaders


The general ray-casting algorithm is applicable to compute shaders. There are no function pointers
or polymorphism techniques to use, so every primitive must be hard coded for intersections
and normals separately. Currently, the supported primitives are spheres and planes. Light
illumination is calculated via the Phong reflection model, which is prettier, but slow and requires
many parameters. Purely reflective surfaces mix a factor of their own surface illumination with
reflected light. Refractive surfaces mix none of their surface illumination, and instead use
Fresnel coefficients to mix refracted and reflected light based on their index of refraction.
To remove antialiasing, subpixel supersampling was used to uniformly sample every pixel and average
over such rays.

Unfortunately, shaders do not support recursion, which is the common method of tracing reflections
and refractions. To overcome this, one may hardcode traces for every depth (and just copy the
function body / use C++ preprocessing to copy them). Alternatively, I built a ray tree which
stores every reflection and refraction ray. First, the tree is built as a binary tree where
each node's children are the refractions / reflections from that ray. Once the tree is built,
the colors are determined in reverse order by adding reflected, refracted, and direct contributions
as desired.

Since this is a toy ray-tracer, I have not heavily optimized its algorithm. Some calculations are
repeated several times in different parts of the code, and some unnecessary computations or
iterations are often done. I've opted for readable and easily modifyable code instead. 

There are also some artefacts / glitches. Intersections of multiple objects often leave jaggedness,
even with supersampling. Overlapping objects with the camera, extreme parameters, etc. may also
cause errors.

__WARNING: RUNNING WITH TOO MANY OBJECTS / LARGE SAMPLING / DEEP TRACING MAY CAUSE CRASHES!__
