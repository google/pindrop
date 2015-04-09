Buses    {#pindrop_guide_buses}
=====

The schema for bus definitions is defined in `schemas/buses.fbs`.  A bus data
file must be supplied by the user which contains a list of Bus defintions and
how they are related. The audio engine will load the bus file specified in the
configuration file.

The main purpose of a bus is to adjust the gain (or volume) of a group of
sounds in tandem.  Any number of buses may be defined.

All [SoundCollectionDef][]s define which bus they are to be played on.  Example
buses might be `music`, `ambient_sound`, `sound_effects` or `voice_overs`.  A
bus may list other buses as `child_buses`, which means that all changes to that
bus's gain level affect all children of that bus as well.  There is always a
single `master` bus. This is the root bus from which all other buses descend.

A bus may also define [duck_buses][], which are buses that will lower in volume
when a sound is played on that bus.  For example, a designer might want to have
background sound effects and music lower in volume when an important dialog is
playing.  To do this the sound effect and music buses would be added to the
`duck_buses` list of the dialog bus.

<br>

  [duck_buses]: http://en.wikipedia.org/wiki/Ducking
  [SoundCollectionDef]: @ref pindrop_guide_sound_collection_def
