Building for Linux    {#pindrop_guide_building_linux}
==================

## Version Requirements

Following are the minimum required versions for the tools and libraries you
need for building [Pindrop][] for Linux:

  * [cmake][]: 2.8.12 or newer
  * [automake][]: 1.141 or newer
  * [autoconf][]: 2.69 or newer
  * [libtool][]: 2.4.2 or newer
  * [Python][]: 2.7.x

## Before Building    {#building_linux_prerequisites}

Prior to building, install the following components using the [Linux][]
distribution's package manager:

  * [cmake][]. You can also manually install from [cmake.org](http://cmake.org).
  * [autoconf][], [automake][], and [libtool][]
  * [Python][]: 2.7.x

For example, on Ubuntu:

    sudo apt-get install cmake libtool automake libtool python

## Building

When building Pindrop, you can either build the library as part of another
project, or you can build it as a stand-alone library. To simply build the
library (and the sample project) do the following:

  * Generate makefiles from the [cmake][] project in the `pindrop` directory.
  * Execute `make` to build the library and sample.

For example:

    cd pindrop
    cmake -G'Unix Makefiles'
    make

To build Pindrop from another project, add the following to your project's
CMakeLists.txt:

    add_subdirectory("${path_to_pindrop}" pindrop)
    include_directories(${path_to_pindrop}/include)
    include_directories(${PINDROP_FLATBUFFERS_GENERATED_INCLUDES_DIR})

Additionally, ensure that you are linking the library and it's dependencies when
running `target_link_libraries`:

    target_link_libraries(
      pindrop
      sdl2-static
      sdl_mixer
      libvorbis
      libogg)

<br>

  [autoconf]: http://www.gnu.org/software/autoconf/
  [automake]: http://www.gnu.org/software/automake/
  [cmake]: http://www.cmake.org/
  [libtool]: http://www.gnu.org/software/libtool/
  [Linux]: http://en.wikipedia.org/wiki/Linux
  [Pindrop]: @ref pindrop_guide_overview
  [Python]: http://www.python.org/download/releases/2.7/

