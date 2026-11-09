# Orbit 0.5 · User guide

Open [Orbit web](https://orbit.avalla.dev) and press Listen. Choose your language in the top bar; the preference persists locally and can change while playing. Presets store musical settings independently.

## Shape the sound

Choose a scale and Glass, Rubber, Mist, Soft Keys or Bell. Edit eight notes and lock favourite steps. Body adds two sine voices detuned ±7 cents and one an octave below, with normalized gain. Try 35–60%. New sessions use 35%; older presets load at zero. Sub and drums have independent levels. Delay and reverb add space.

## Compose and export

Flow develops phrases; Capture preserves notes, rests, durations and drums. Swing delays alternate eighth notes. Probability chooses events; locked notes still respect rests. Energy is neutral at 50%; Fill adds a bar of drum variation. Save four scenes and recall them at the next 4/4 bar during playback. JSON preserves all scenes without audio.

Set BPM on the web or Internal tempo in the app. With DAW sync enabled, use host tempo and transport. MIDI exports synth, sub and GM drums, excluding effects, probability and Fill. Record the mix or three aligned dry stereo WAVs, 16 bit, maximum five minutes. Download before closing the browser; in the plugin end recording with its own button.

## Install and troubleshoot

[Download macOS Intel + Apple Silicon](https://github.com/avalla/luma-orbit/releases/latest/download/Luma-Orbit-macOS-Universal.zip). Copy VST3 to `~/Library/Audio/Plug-Ins/VST3/` or AU to `~/Library/Audio/Plug-Ins/Components/`, then rescan. Builds have ad-hoc signatures and are not notarized. Open old `.orbit` files in the plugin and save as JSON. Web and native share musical state, with different audio engines. Host parameter names and system dialogs follow host/OS conventions.

If silent, check levels, probability and transport. On iPhone check media volume and Bluetooth/AirPlay, open Safari directly and press Listen again. Physical speaker validation remains separate from desktop tests.
