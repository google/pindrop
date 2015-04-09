Listener    {#pindrop_guide_listener}
========

A `Listener` is like a person's head. It represents a point in space that
listens for positional audio. The closer the Listener is to audio, the louder it
gets. When positional audio is playing, the volume of the audio depends only on
its distance from the nearest `Listener`. If there are no active `Listeners`,
then all positional audio will be silent.

`Listeners` are allocated during the [AudioEngine][]'s initialization. The total
number of `Listeners` are specified by the [AudioConfig][].

You can create a `Listener` like so:

    Listener listener = audio_engine_.AddListener();

Once you have a `Listener`, you can set it's location with `SetLocation()`. The
location of the `Listener` can be updated as often as is necessary.

    listener.SetLocation(GetSomeLocation());

When a `Listener` is no longer needed it can be removed like so:

    audio_engine_.RemoveListener(&listener);

In our initial release, `Listener` orientation is not supported. We hope to add
this in a release in the near future.

<br>

  [AudioConfig]: @ref pindrop_guide_audio_config
  [AudioEngine]: @ref pindrop_guide_audio_engine
