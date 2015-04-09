Channel    {#pindrop_guide_channel}
=======

A `Channel` tracks the current state of a single playing sample or stream of
audio. Every playing sound is associated with one `Channel`. The total number of
valid channels is determined by the [AudioConfig][]. Allocating more channels
than necessary may result in wasted memory, and if too many channels are played
at once the mixing thread might not be able to keep up, resulting in stuttering
audio.

A `Channel` object allows you to stop or change the location of some currently
playing audio.  `Channel`s that are not playing a looping sound will terminate
on their own and require no manual clean up.

Attempting to play a new sound when there are no remaining channels will cause
the lowest priority channel to be halted to make room for the new channel.

Here is an example of how a `Channel` might be used:

    Channel music_channel; // Not initialized; does not currently point to any
                           // playing channel.
    motor_channel = audio_engine_.PlaySound(motor_sound_handle);
    motor_channel.SetLocation(MotorBoatLocation());
    // ... Some time later
    motor_channel.Stop();

An uninitialized channel is considered invalid, and cannot be used until it has
been been initialized.  The function `Valid()` returns true when the `Channel`
object has been successfully initialized.

<br>

  [AudioConfig]: @ref pindrop_guide_audio_config
