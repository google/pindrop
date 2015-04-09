AudioEngine    {#pindrop_guide_audio_engine}
===========

The `AudioEngine` is the main interface to library.  The `AudioEngine` is
responsible for loading and unloading [SoundBank][]s, playing sounds, managing
channel priorities, spawning and despawing [Listener][]s, and more.

### Initialization

To initialize the `AudioEngine`, you must specify an [AudioConfig][]. You may
either load one yourself or supply a path to an [AudioConfig][] FlatBuffer
binary.  For example:

~~~{.cpp}
    audio_engine_.Initialize("audio_config.bin");
~~~

Alternatively, you can also do this:

~~~{.cpp}
    const AudioConfig* audio_config = GetAudioConfig();
    audio_engine_.Initialize(audio_config);
~~~

Once the library is initialized, the AudioConfig object is no longer necessary
and can be deallocated.

### Main Loop

Once during every game loop there should be a call to AdvanceFrame. This is
where the `AudioEngine` performs all prioritization and gain adjustments.

~~~{.cpp}
    while (!GameOver()) {
      // ...
      float delta_time = SecondsSinceLastFrame();
      audio_engine_.AdvanceFrame(delta_time);
      // ...
    }
~~~

### Loading and Unloading Audio

Before any audio can be played, the associated [SoundBank][] must be loaded into
memory. Loading a [SoundBank][] loads all [SoundCollectionDef][]s listed in the
file, making them available to be played.

To load a `SoundBankDef` simply call

~~~{.cpp}
    audio_engine_.LoadSoundBank("path/to/soundbank.bin");
~~~

And when the [SoundBank][] is no longer required you can unload it with

~~~{.cpp}
    audio_engine_.UnloadSoundBank("path/to/soundbank.bin");
~~~

### Playing Audio

Once a [SoundCollectionDef][] has been loaded, it may be played with the
following code:

~~~{.cpp}
    Channel music_channel = audio_engine_.PlaySound("MenuMusic");
~~~

However, playing a sound in this way incurs a map lookup. If a sound is going
to be played frequently, a handle may be cached to improve performance.

~~~{.cpp}
    SoundHandle menu_music = audio_engine_.GetSoundHandle("MenuMusic");
    Channel music_channel = audio_engine_.PlaySound(menu_music);
~~~

### Positional and Nonpositional Audio

[SoundCollectionDef][]s may be either positional or non-positional. When they
are positional, the loudness of a channel is determined by its distance from the
nearest [Listener][]. Typically you will only need a single [Listener][],
positioned at the camera, but in some cases (such as games with split screen
multiplayer for example) it may be desirable to have more than one [Listener][].
When there are more than one [Listener][]s, only the distance from the nearest
listener is considered.  If there are no [Listener][]s at all, all positional
audio will be silent.

The maximum number of listeners is specified in the [AudioConfig][]. Listeners
can be spawned as follows:

~~~{.cpp}
    Listener listener = audio_engine_.AddListener();
~~~

The location of the listener can be updated as often as is necessary. When a
Listener is no longer needed it can be removed like so:

~~~{.cpp}
    audio_engine_.RemoveListener(&listener);
~~~

<br>

  [AudioConfig]: @ref pindrop_guide_audio_config
  [Listener]: @ref pindrop_guide_listener
  [SoundBank]: @ref pindrop_guide_sound_bank
  [SoundCollectionDef]: @ref pindrop_guide_sound_collection_def

