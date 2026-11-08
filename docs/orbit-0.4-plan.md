# Orbit 0.4 implementation

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
