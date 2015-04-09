Building for OS X    {#pindrop_guide_building_osx}
=================

You can use [cmake][] to generate an [Xcode][] project for Pindrop on [OS X][].

# Version Requirements

These are the minimum required versions for building Pindrop on OS X:

  * [OS X][]: Mavericks 10.9.1.
  * [Xcode][]: 5.1.1 or newer
  * [cmake][]: 2.8.12 or newer
  * [Python][]: 2.7.x

# Before Building    {#building_osx_prerequisites}

# Creating the Xcode project using cmake

The [Xcode][] project is generated using [cmake][].

For example, the following generates the [Xcode][] project in the `pindrop`
directory.

~~~{.sh}
    cd pindrop
    cmake -G "Xcode"
~~~

# Building with Xcode

  * Double-click on `pindrop/pindrop.xcodeproj` to open the project in
    [Xcode][].
  * Select "Product-->Build" from the menu.

You can also build the game from the command-line.

  * Run `xcodebuild` after generating the Xcode project to build all targets.
  * You may need to force the `generated_includes` target to be built first.

For example, in the pindrop directory:

~~~{.sh}
    xcodebuild -target generated_includes
    xcodebuild
~~~

<br>

  [cmake]: http://www.cmake.org
  [OS X]: http://www.apple.com/osx/
  [Pindrop]: @ref pindrop_guide_overview
  [Python]: http://www.python.org/download/releases/2.7/
  [Xcode]: http://developer.apple.com/xcode/
