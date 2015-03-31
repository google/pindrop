Pindrop      {#pindrop_index}
========

[Pindrop][] manages playback of both sounds and music.  It is largely
data-driven.  In a config file, settings such as the number of output channels
and the buffer size can be adjusted.  The audio engine will manage these
channels, ensuring that if this limit is exceeded only the lowest priority
sounds are dropped.

## Motivation

TODO

## Dependencies

[FlatBuffers][], a fast serialization system is used to store the
game's data. The game configuration data is stored in [JSON][] files
which are converted to FlatBuffer binary files using the flatc compiler.

[MathFu][], a geometry math library optimized for [ARM][] and [x86][]
processors. Pindrop uses [MathFu][] three dimensional vectors for distance
attenuation calculations.

In addition, [fplutil][] is used to build, deploy, and run the game,
build and archive the game, and profile the game's CPU performance.

## Functionality

TODO

## Supported Platforms

[Pindrop][] has been tested on the following platforms:

   * [Nexus Player][], an [Android TV][] device
   * [Android][] phones and tablets
   * [Linux][] (x86_64)
   * [OS X][]
   * [Windows][]

We use [SDL][] as our cross platform layer.
The library is written entirely in C++. The game can be compiled using
[Linux][], [OS X][] or [Windows][].

## Download

[Pindrop][] can be downloaded from:
   * [GitHub][] (source)
   * [GitHub Releases Page](http://github.com/google/pindrop/releases) (source)

**Important**: Pindrop uses submodules to reference other components it
depends upon, so download the source from [GitHub][] using:

~~~{.sh}
    git clone --recursive https://github.com/google/pindrop.git
~~~

## Feedback and Reporting Bugs

   * Discuss Pindrop with other developers and users on the
     [Pindrop Google Group][].
   * File issues on the [Pindrop Issues Tracker][].
   * Post your questions to [stackoverflow.com][] with a mention of **pindrop**.

<br>

  [Android]: http://www.android.com
  [Android TV]: http://www.android.com/tv/
  [ARM]: http://en.wikipedia.org/wiki/ARM_architecture
  [FlatBuffers]: http://google-opensource.blogspot.ca/2014/06/flatbuffers-memory-efficient.html
  [fplutil]: http://android-developers.blogspot.ca/2014/11/utilities-for-cc-android-developers.html
  [JSON]: http://www.json.org/
  [Linux]: http://en.m.wikipedia.org/wiki/Linux
  [MathFu]: http://googledevelopers.blogspot.ca/2014/11/geometry-math-library-for-c-game.html
  [Nexus Player]: http://www.google.com/nexus/player/
  [OS X]: http://www.apple.com/osx/
  [Pindrop]: @ref pindrop_index
  [Pindrop Google Group]: http://group.google.com/group/pindrop
  [Pindrop Issues Tracker]: http://github.com/google/pindrop/issues
  [SDL]: https://www.libsdl.org/
  [stackoverflow.com]: http://www.stackoverflow.com
  [Windows]: http://windows.microsoft.com/
  [x86]: http://en.wikipedia.org/wiki/X86
  [WebP]: https://developers.google.com/speed/webp
  [GitHub]: http://github.com/google/pindrop

