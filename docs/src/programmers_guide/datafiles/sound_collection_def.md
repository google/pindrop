SoundCollectionDef    {#pindrop_guide_sound_collection_def}
==================

The `SoundCollectionDef` schema is defined in `sound_collection_def.fbs.`
Individual `SoundCollectionDef`s are defined in the `sounds/` subdirectory, one
`SoundCollectionDef` per file. `SoundCollectionDef`s are the primary method of
playing audio through the [AudioEngine][].

A `SoundCollectionDef`, as its name suggests, is a collection of related sounds.
For example, you may have multiple samples of a foot step, or songs you would
like to randomly select to play on a menu screen. A trivial `SoundCollection`
may contain only a single audio sample. Sample playback probability may be
weighted to give preference to certain samples. Each `SoundCollectionDef` must
have a unique identifier, and must have been loaded by a [SoundBankDef][] prior
to being played.

The SoundCollectionDef schema also specifies a number of other fields which are
documented in the schema itself.

<br>

  [AudioEngine]: @ref pindrop_guide_audio_engine
  [SoundBankDef]: @ref pindrop_guide_sound_bank_def
