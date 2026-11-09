# Orbit 0.4 implementation

Status: completed and released on 2026-11-08. This document records the implementation;
current user instructions live in the [quickstart](Orbit-quickstart.md) and changes in the [changelog](../CHANGELOG.md).

## Product contract

- Four named scene slots save the complete musical idea; recall at the next 4/4 bar while playing, immediately while stopped. Saving a slot is explicit.
- Swing delays alternate eighths without changing bar length. Probability affects unlocked melodic events and drum hits. Defaults preserve existing playback.
- Energy is a non-destructive macro around a neutral midpoint, shaping brightness, exploration, rhythmic density without changing master gain.
- Capture retains notes, rests, durations and percussion. Restart and session restore retain the captured phrase.
- Shared, validated JSON presets carry the same controls and patterns between web and native. Older presets remain readable. Synthesis differs across engines; interchange promises musical state, not identical waveforms.
- MIDI exports the current phrase as melody, sub and GM drums with tempo and swing. WAV stems capture simultaneous dry synth/sub/drums; the existing WAV captures the effected mix.
- Fill is a temporary one-bar drum variation; it does not rewrite the saved pattern.

## Implementation and validation

1. Introduce shared schema and timing/export helpers; migrate web state and controls.
2. Add native pattern snapshots, performance controls and scene handoff without allocating or waiting in the audio callback.
3. Add MIDI and simultaneous stem recording in both versions; expose readable controls.
4. Test schema rejection/roundtrips, capture restart, swing bar duration, probability, scene boundaries, MIDI events and stem alignment.
5. Build universal native formats, inspect UI and browser playback, update documentation, commit/push, publish release and deploy web.

Physical iPhone output and third-party DAW compatibility still require hardware/host verification.

## Validation completed

- Universal VST3, AU and standalone compiled; all three CTest targets passed. Orbit tests also passed as x86_64 under Rosetta.
- Native tests cover complete phrase restart/DAW restoration, quantized scene recall, neutral legacy migration, bounded one-bar Fill, probability and swing, readable MIDI and equal-length stems.
- Web tests cover schema migration/rejection, nested scene rejection, parsed MIDI events, swing bar length and synchronized PCM streams with the five-minute limit.
- A shared web fixture loads in native; the native-generated JSON with two scenes validates in the web test. CI now checks this after native tests.
- Browser playback, queued scene recall and simultaneous mix/stem recording verified. All three stem files contained audio and had exactly 2,622,208 frames at 48 kHz in the browser test.
- Native scene UI inspected; mobile browser layout at 390 px has no horizontal overflow, 44 px scene buttons and 16 px text. Browser console had no errors or warnings.

## Boundaries

Audio callbacks copy only fixed-size snapshots and use nonblocking lock attempts; JSON, file dialogs and parameter notifications remain outside the callback. Scene audio changes happen on the sample clock; the message-thread timer mirrors controls into host parameters. WAV disk writes use background queues, with explicit overflow reporting. Empty scene slots are intentional; the user saves an idea before recalling it.

## Delivery

- Source: commit `6b73b383d79d7eca71c40f824a4b8f52d3d5a767`, tag `orbit-v0.4.0`.
- [Release pipeline](https://github.com/avalla/luma-orbit/actions/runs/42527775440): successful, including native/web interchange and packaging.
- [Public release](https://github.com/avalla/luma-orbit/releases/tag/orbit-v0.4.0): downloaded ZIP verified for integrity, version 0.4.0 and universal binaries in all three formats.
- [Web deployment](https://orbit.avalla.dev): Cloudflare version `2089b6aa-2993-4275-b02f-403f644a474a`; production playback and three non-silent, equal-length WAV stems verified.
