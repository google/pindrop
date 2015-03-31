Building for Windows    {#pindrop_guide_building_windows}
====================

You can use [cmake][] to generate a [Visual Studio][] project for
[Pindrop][] on [Windows][].

# Version Requirements

These are the minimum required versions for building [Pindrop][] for Windows:

-   [Windows][]: 7
-   [Visual Studio][]: 2010 or newer
-   [cmake][]: 2.8.12 or newer.
-   [Python][]: 2.7.*

# Before Building    {#building_windows_prerequisites}

-   Install [cmake][]
-   Install [Python][]

# Creating the Visual Studio Solution using cmake

Use [cmake][] to generate the [Visual Studio][] solution and project files.

The following example generates the [Visual Studio][] 2010 solution in the
`pindrop` directory:

    cd pindrop
    cmake -G "Visual Studio 10"


Running [cmake][] under [cygwin][] requires empty TMP, TEMP, tmp and temp
variables.  To generate a [Visual Studio][] solution from a [cygwin][]
bash shell use:

    $ cd pindrop
    $ ( unset {temp,tmp,TEMP,TMP} ; cmake -G "Visual Studio 10" )

# Building with Visual Studio

-   Double-click on `pindrop/pindrop.sln` to open the solution.
-   Select "Build-->Build Solution" from the menu.

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
  [Visual Studio]: http://www.visualstudio.com/
  [Windows]: http://windows.microsoft.com/
  [DirectX SDK]: http://www.microsoft.com/en-us/download/details.aspx?id=6812
  [Python]: http://www.python.org/download/releases/2.7/
