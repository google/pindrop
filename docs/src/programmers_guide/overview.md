Overview    {#pindrop_guide_overview}
========

## Downloading

[Pindrop][] can be downloaded from [GitHub](http://github.com/google/pindrop)
or the [releases page](http://github.com/google/pindrop/releases).

~~~{.sh}
    git clone --recursive https://github.com/google/pindrop.git
~~~

## Important Classes

The Pindrop API contains the following important classes:
* [AudioEngine][]
* [Channel][]
* [Listener][]

Pindrop is built on top of [SDL][], a low-level cross platform library.
[SDL][] abstracts input, file loading, threading, system events, logging, and
other systems from the underlying operating system.

## Source Layout

The following bullets describe the directory structure of the game.

| Path                          | Description                                  |
|-------------------------------|----------------------------------------------|
| `pindrop` base directory      | Project build files and run script.          |
| `assets`                      | Assets loaded by the demo project.           |
| `jni`                         | The android makefile for the library.        |
| `schemas`                     | Schemas for [FlatBuffers][] data.            |
| `scripts`                     | Various utility scripts.                     |
| `src`                         | The library's source.                        |
| `src/rawdata`                 | JSON [FlatBuffers] data for the demo.        |

<br>

  [AudioEngine]: @ref pindrop_guide_audio_engine
  [Channel]: @ref pindrop_guide_channel
  [Listener]: @ref pindrop_guide_listener
  [Engine]: @ref pindrop_guide_engine
  [FlatBuffers]: http://google-opensource.blogspot.ca/2014/06/flatbuffers-memory-efficient.html
  [fplutil]: http://google.github.io/fplutil
  [GUI]: @ref pindrop_guide_gui
  [Impel]: @ref pindrop_guide_impel
  [MathFu]: http://googledevelopers.blogspot.ca/2014/11/geometry-math-library-for-c-game.html
  [Nexus Player]: http://www.google.com/nexus/player/
  [Pindrop]: @ref pindrop_guide_audio
  [SDL]: https://www.libsdl.org/
  [SDL-mixer]: http://www.libsdl.org/projects/SDL_mixer/
