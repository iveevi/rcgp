# RCGP: Resource Contracts for Graphics Programming

RCGP is a pure-C++ graphics programming framework based on the SIGGRAPH 2026 paper [*RCGP: Resource Contracts for Graphics Programming*](https://iveevi.github.io/rcgp/). It establishes a type system for resources in graphics applications, such as storage buffers and textures, that enables static verification of resource usage in both shader code and host code.

<p align="center">
  <img src="https://iveevi.github.io/rcgp/teaser.png" width="100%" />
</p>

# Usage

RCGP is designed to be usable in any Vulkan project using C++26 and beyond, and is supported on GCC 15+ and Clang 19+ compilers.

To use the framework, do the following:

```cpp
#include <rcgp.hpp>

// Recommended, especially for shader code
using namespace rcgp;
```

This repository does not come with batteries included; users will need to provide the following dependencies:
* Vulkan, for the core graphics backend.
* [glslang](https://github.com/KhronosGroup/glslang), for compiling the transpiled GLSL shader modules to SPIR-V.
* [fmt](https://github.com/fmtlib/fmt), for various IO utilities.
* [glfw](https://github.com/glfw/glfw), for OS-window management.
* Python 3, used for parts of the build process.

Optionally, to enable automatic conversions to [glm](https://github.com/g-truc/glm) matrix and vector types, define `RCGP_SUPPORT_GLM` before the inclusion of `rcgp.hpp` and, of course, include the glm library itself.

Samples and documentation will be coming soon!