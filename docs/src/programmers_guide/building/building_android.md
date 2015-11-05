Building for Android    {#pindrop_guide_building_android}
====================

## Version Requirements

Following are the minimum tested versions for the tools and libraries you
need for building [Pindrop][] for Android:

  * [Android SDK][]: Android 5.0 (API Level 21)
  * [ADT][]: 20140702
  * [Android NDK][]: android-ndk-r10e
  * [cmake][]: 2.8.12 or newer
  * [Python][]: 2.7.x

## Before Building

  * Install prerequisites for the developer machine's operating system.
      * [Linux prerequisites](@ref building_linux_prerequisites)
      * [OS X prerequisites](@ref building_osx_prerequisites)
      * [Windows prerequisites](@ref building_windows_prerequisites)
  * Install [fplutil prerequisites][]
  * Install the [Android SDK][].
  * Install the [Android NDK][].

## Building

To include [Pindrop][] in your project you will need to make the following
changes to your Android.mk

  * Add `libpindrop` to your list of `LOCAL_STATIC_LIBRARIES`
  * Run `$(call import-add-path,$(DEPENDENCIES_MOTIVE_DIR)/..)` near the end of
    Android.mk, after you call `include $(BUILD_SHARED_LIBRARY)`
  * Lastly, run `$(call import-module,audio_engine/jni)`

Your project should now build and link against Pindrop.

<br>

  [ADT]: http://developer.android.com/tools/sdk/eclipse-adt.html
  [Android NDK]: http://developer.android.com/tools/sdk/ndk/index.html
  [Android SDK]: http://developer.android.com/sdk/index.html
  [cmake]: http://www.cmake.org/
  [fplutil prerequisites]: http://google.github.io/fplutil/fplutil_prerequisites.html
  [fplutil]: http://google.github.io/fplutil
  [Pindrop]: @ref pindrop_guide_overview
  [Python]: http://www.python.org/download/releases/2.7/
