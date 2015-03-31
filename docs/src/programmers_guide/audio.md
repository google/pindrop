Pindrop {#pindrop_guide}
=====

# Overview {#pindrop_guide_overview}

The audio engine manages playing both sounds and music.  It is largely
data-driven.  In a config file, settings such as the number of output channels
and the buffer size can be adjusted.  The audio engine will manage these
channels, ensuring that if this limit is exceeded only the lowest priority
sounds are dropped.

# Sound Collections {#pindrop_guide_sound_collections}

Individual sounds are represented by `SoundSource` objects.  A `SoundSource` is
not played directly though.  `SoundSource`s are gathered into a class called
`SoundCollection` which represents a collection of related sounds.  For
example, in Pie Noon there are many different sounds for a pie hitting a
character to avoid the monotony of every hit sounding the same.

Each `SoundCollection` has a unique identifier.  To play a given
`SoundCollection`, its identifier is passed to `AudioEngine::PlaySound`.

# Buses {#pindrop_guide_buses}

The main purpose of a bus is to adjust the gain (or volume) of a group of
sounds in tandem.  Any number of buses may be defined.

All `SoundCollection`s define which bus they are to be played on.  Examples
buses might be 'music', 'ambient_sound', 'sound_effects'.  A bus may list other
buses as 'child_buses', which means that all changes to that bus's gain level
affect all children of that bus as well.  There is always a single 'master'
bus, which is the root bus which all other buses decend from.  A bus may also
define 'duck_buses', which are buses that will lower in volume when a sound is
played on that bus.  For example, a designer might want to have background
sound effects and music lower in volume when an important dialog is playing.
To do this the sound effect and music buses would be added to the 'duck_buses'
list in the dialog bus.

# Configuration {#pindrop_guide_configuration}

The following configuration files control the behavior of the audio engine.

| JSON File(s)                 | Flatbuffers Schema                            |
|------------------------------|-----------------------------------------------|
| `config.json`                | `audio_config.fbs` (included in `config.fbs`) |
| `buses.json`                 | `buses.fbs`                                   |
| `sound_assets.json`          | `sound_assets.fbs`                            |
| `sounds/*.json`              | `sound_collection_def.fbs`                    |
