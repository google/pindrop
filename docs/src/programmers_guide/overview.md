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
  * [AudioEngine][] - The engine that manages the audio channels.
  * [Channel][] - An object that tracks the state of a single stream or sample.
  * [Listener][] - A positional object which determines the volume of a sound.

As well as these important schemas:
  * [AudioConfig][] - The basic configuration for the engine.
  * [Buses][] - A set of interrelated buses which can adjust channel gains
                automatically.
  * [SoundBankDef][] - A set of [SoundCollectionDef][]s that are to be loaded
                       and unloaded in tandem.
  * [SoundCollectionDef][] - A set of one or more related sounds that can be
                             played.

[Pindrop][] is built on top of [SDL][], a low-level cross platform library.
[SDL][] abstracts input, file loading, threading, system events, logging, and
other systems from the underlying operating system.

## Source Layout

The following bullets describe the directory structure of the library.

| Path                          | Description                                  |
|-------------------------------|----------------------------------------------|
| `pindrop` base directory      | Project build files and run script.          |
| `assets`                      | Assets loaded by the sample project.         |
| `jni`                         | The android makefile for the library.        |
| `schemas`                     | Schemas for [FlatBuffers][] data.            |
| `scripts`                     | Various utility scripts.                     |
| `src`                         | The library's source.                        |
| `samples`                     | Source for a sample project.                 |
| `samples/rawdata`             | JSON [FlatBuffers] data for the sample.      |

<br>

  [AudioConfig]: @ref pindrop_guide_audio_config
  [AudioEngine]: @ref pindrop_guide_audio_engine
  [Buses]: @ref pindrop_guide_buses
  [Channel]: @ref pindrop_guide_channel
  [Engine]: @ref pindrop_guide_engine
  [FlatBuffers]: http://google-opensource.blogspot.ca/2014/06/flatbuffers-memory-efficient.html
  [fplutil]: http://google.github.io/fplutil
  [Listener]: @ref pindrop_guide_listener
  [MathFu]: http://googledevelopers.blogspot.ca/2014/11/geometry-math-library-for-c-game.html
  [Pindrop]: @ref pindrop_guide_overview
  [SDL-mixer]: http://www.libsdl.org/projects/SDL_mixer/
  [SDL]: https://www.libsdl.org/
  [SoundBankDef]: @ref pindrop_guide_sound_bank_def
  [SoundCollectionDef]:  @ref pindrop_guide_sound_collection_def
