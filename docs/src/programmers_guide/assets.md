Modifying Assets {#pindrop_guide_assets}
================

### Overview

The library has many data driven components allowing content creators to add
assets.

Data is specified by [JSON][] files in `pindrop/src/rawassets` which are
converted to binary and read in-game using [Flatbuffers][].  Each [JSON][]
file is described by a [Flatbuffers schema][] in `schemas`.

The following table maps each set of [JSON][] files in `schemas` to
[Flatbuffers][] schemas in `src/flatbufferschemas`:

### Building

The game loads assets from the `assets` directory.  In order to convert data
into a format suitable for the game runtime, assets are built by a Python
script which requires:

*   [Python][] to run the asset build script. [Windows][] users may need to
    install [Python][] and add the location of the python executable to the PATH
    variable.  For example in Windows 7:
    *   Right click on `My Computer`, select `Properties`.
    *   Select `Advanced system settings`.
    *   Click `Environment Variables`.
    *   Find and select `PATH`, click `Edit...`.
    *   Add `;%PATH_TO_PYTHONEXE%` where `%PATH_TO_PYTHONEXE%` is the location
        of the python executable.
*   `flatc` ([Flatbuffers compiler][]) to convert [JSON][] text files to .bin
    files in the `pindrop/assets` directory.

After modifying the data in the `pindrop/src/rawassets` directory, the assets
need to be rebuilt by running the following command:

    cd pindrop
    python scripts/build_assets.py

Each file under `src/rawassets` will produce a corresponding output file in
`assets`.  For example, after running the asset build
`assets/config.bin` will be generated from `src/rawassets/config.json`.

### Audio

`SoundId` values in `src/flatbufferschemas/pindrop_common.fbs` are used to
associate game events (states) in the *Character State Machine* with sounds.

`src/rawassets/sound_assets.json` associates `SoundId` values with sounds
(`SoundDef`, see `src/flatbufferschemas/sound.fbs`).  Each entry in the
`sounds` list references a .bin file in the `assets` directory generated from
a [JSON][] file in `src/rawassets/sounds`.  For example, to load
`src/rawassets/sounds/throw_pie.json` the `sounds` list should reference
`sounds/throw_pie.bin`.

Each sound (`SoundDef`) can reference a set of audio samples, where each
filename references a sample in the `assets` directory.  For example, to load
`assets/sounds/smack01.wav` the string `sounds/smack01.wav` should be added
to the `filename` entry of an `audio_sample` in the `audio_sample_set` list.

<br>

  [Flatbuffers]: http://google.github.io/flatbuffers/
  [Flatbuffers compiler]: http://google.github.io/flatbuffers/md__compiler.html
  [Flatbuffers schema]: http://google.github.io/flatbuffers/md__schemas.html
  [JSON]: http://json.org/
  [Python]: http://python.org/
  [webp]: https://developers.google.com/speed/webp/
  [Windows]: http://windows.microsoft.com/
