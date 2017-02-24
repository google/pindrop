Pindrop    {#pindrop_index}
=======

[Pindrop][] is an audio engine designed with the needs of games in mind.  It's
not just an audio mixer, it also manages the prioritization, adjusts gain based
distance,  manages buses, and more. It is largely data-driven.  The library user
needs only to load a handful of configuration files and Pindrop handles the
rest.

## Features

### Sound Bank Management

To save memory, only load the sounds you need by grouping them into sound banks.
Load as many banks as you need. Samples are reference counted, so you'll never
load the same sample multiple times.

### Channel Prioritization

In a busy scene a game might have thousands of samples playing at the same time.
Sometimes that is more than the game can handle. Pindrop will manage the
priority of each audio channel and only drop the least important channels when
too many simultaneous streams play at once.

### Distance Attenuation

[Pindrop][] supports positional and non-positional sounds. Positional sounds can
be set to attenuate based on their distance from the listener. The attenuation
curve can be customized so that the audio rolls off with the curve that sounds
right.

### Buses

A series of hierarchical buses may be specified that can automatically adjust
channel gain on the fly. If there is an important sound or voice over that needs
to be heard, audio playing on less important buses can be [ducked][] to ensure
the player hears what they need.

### Easy to Use

[Pindrop][] only requires a few [JSON][] based configuration files, and the code
required to hook it into your own game engine is minimal. File loading is made
fast by [FlatBuffers][]. All you need to do is initialize the library and run
`AdvanceFrame` once each frame in your main loop.

## Dependencies

[FlatBuffers][], a fast serialization system, is used to store the game's data.
The game configuration data is stored in [JSON][] files which are converted to
FlatBuffer binary files using the flatc compiler.

[MathFu][], a geometry math library optimized for [ARM][] and [x86][]
processors. Pindrop uses [MathFu][] three dimensional vectors for distance
attenuation calculations.

[SDL Mixer][], an audio mixing library that sits on top of [SDL][]. SDL Mixer
handles mixing the playing audio samples and streams that Pindrop manages.  SDL
Mixer in turn depends on [libogg][] and [libvorbis][] as well.

In addition, [fplutil][] is used to build, deploy, and run the game; build and
archive the game; and profile the game's CPU performance.

## Supported Platforms

[Pindrop][] has been tested on the following platforms:

   * [Nexus Player][], an [Android TV][] device
   * [Android][] phones and tablets
   * [Linux][] (x86_64)
   * [OS X][]
   * [Windows][]

We use [SDL][] as our cross platform layer.

[Pindrop][] is written entirely in C++. The library can be compiled using
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
   * Post your questions to [stackoverflow.com][] with a mention of
     **fpl pindrop**.

<br>

  [Android TV]: http://www.android.com/tv/
  [Android]: http://www.android.com
  [ducked]: http://en.wikipedia.org/wiki/Ducking
  [FlatBuffers]: http://google-opensource.blogspot.ca/2014/06/flatbuffers-memory-efficient.html
  [fplutil]: http://android-developers.blogspot.ca/2014/11/utilities-for-cc-android-developers.html
  [GitHub]: http://github.com/google/pindrop
  [JSON]: http://www.json.org/
  [Linux]: http://en.m.wikipedia.org/wiki/Linux
  [MathFu]: http://googledevelopers.blogspot.ca/2014/11/geometry-math-library-for-c-game.html
  [Nexus Player]: http://www.google.com/nexus/player/
  [OS X]: http://www.apple.com/osx/
  [Pindrop Google Group]: http://groups.google.com/group/pindrop
  [Pindrop Issues Tracker]: http://github.com/google/pindrop/issues
  [Pindrop]: @ref pindrop_index
  [SDL]: https://www.libsdl.org/
  [stackoverflow.com]: http://stackoverflow.com/search?q=fpl+pindrop
  [stackoverflow.com]: http://www.stackoverflow.com
  [WebP]: https://developers.google.com/speed/webp
  [Windows]: http://windows.microsoft.com/
  [x86]: http://en.wikipedia.org/wiki/X86

