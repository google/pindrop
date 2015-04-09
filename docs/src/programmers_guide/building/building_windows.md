Building for Windows    {#pindrop_guide_building_windows}
====================

You can use [CMake][] to generate a [Visual Studio][] project for
[Pindrop][] on [Windows][].

# Version Requirements

These are the minimum required versions for building [Pindrop][] for Windows:

  * [Windows][]: 7
  * [Visual Studio][]: 2010 or newer
  * [CMake][]: 2.8.12 or newer.
  * [Python][]: 2.7.x

# Before Building    {#building_windows_prerequisites}

  * Install [CMake][]
  * Install [Python][]

# Creating the Visual Studio Solution using CMake

Use [CMake][] to generate the [Visual Studio][] solution and project files.

The following example generates the [Visual Studio][] 2010 solution in the
`pindrop` directory:

    cd pindrop
    cmake -G "Visual Studio 10"

# Building with Visual Studio

  * Double-click on `pindrop/pindrop.sln` to open the solution.
  * Select "Build-->Build Solution" from the menu.

It's also possible to build from the command line using msbuild after using
vsvars32.bat to setup the [Visual Studio][] build environment.  For example,
assuming [Visual Studio][] is installed in
`c:\Program Files (x86)\Microsoft Visual Studio 10.0`.

    cd pindrop
    "c:\Program Files (x86)\Microsoft Visual Studio 11.0\Common7\Tools\vsvars32.bat"
    cmake -G "Visual Studio 10"
    msbuild pindrop.sln

<br>

  [CMake]: http://www.cmake.org
  [Pindrop]: @ref pindrop_guide_overview
  [Python]: http://www.python.org/download/releases/2.7/
  [Visual Studio]: http://www.visualstudio.com/
  [Windows]: http://windows.microsoft.com/
