Modifying Assets    {#pindrop_guide_assets}
================

### Overview

The library has many data driven components allowing content creators to add
assets.  Data is specified by [JSON][] files in `pindrop/src/rawassets` which
are converted to binary and read in-game using [Flatbuffers][].  Each [JSON][]
file is described by a [Flatbuffers schema][] in `schemas`.

When using Pindrop in your own library, a build step will be required to
convert [JSON][] data files into Flatbuffer binary files. A sample project is
provided with pindrop which uses the [Python][] script
`scripts/build_assets.py` to accomplish this task.  This script requires:

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

<br>

  [Flatbuffers compiler]: http://google.github.io/flatbuffers/md__compiler.html
  [Flatbuffers schema]: http://google.github.io/flatbuffers/md__schemas.html
  [Flatbuffers]: http://google.github.io/flatbuffers/
  [JSON]: http://json.org/
  [Python]: http://python.org/
  [webp]: https://developers.google.com/speed/webp/
  [Windows]: http://windows.microsoft.com/
