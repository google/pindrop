SoundBankDef         {#pindrop_guide_sound_bank_def}
============

Another file, sounds_assets.json, is used to load these individual
`SoundCollection`s.  Filenames listed in this JSON file will be loaded in
order.  The index of a sound collection in this file represents the unique
identifier used to play a that sound collection.   E.g., the first sound
sound collection listed would be played by calling
`audio_engine.PlaySound(0)`.
