# Repository Guidelines

## Project Structure & Module Organization

- `Source/`: C++17/JUCE synth, parameters, voices, processor, and editor. `Source/Rms/` contains the independent RMS plugin.
- `Tests/`: native console tests registered with CTest.
- `prototypes/orbit/`: standalone HTML/CSS/JavaScript instrument. `flow.js` generates phrases, `clock.js` schedules audio, `drums.js` synthesizes percussion, and `orbit.js` connects playback and UI.
- `Audio/`: generated MP3 test fixture. `docs/` contains installation notes.
- `scripts/`: packaging and web staging utilities. `PRODUCT.md` and `DESIGN.md` describe UI intent.
- `build/` and `build-*/`: ignored outputs; never commit compiled plugins.

## Build, Test, and Development Commands

Run commands from the repository root. Native builds require macOS, Xcode Command Line Tools, and CMake 3.22+. JUCE is pinned and fetched automatically; use `-DJUCE_SOURCE_DIR=/path/to/JUCE` for a local checkout.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 8
ctest --test-dir build --output-on-failure
node prototypes/orbit/flow.test.cjs
node prototypes/orbit/clock.test.cjs
python3 -m http.server 8765 --directory prototypes/orbit
```

The commands configure/build native targets, run native and web logic tests, and serve Orbit at `http://localhost:8765`. Orbit needs no npm installation or frontend build.

## Coding Style & Naming Conventions

Match surrounding formatting: native C++ generally uses four-space indentation; web JavaScript uses compact one-space nesting; Python uses four spaces. Use PascalCase for classes and camelCase for JavaScript functions and variables. Preserve existing parameter IDs for saved-session compatibility.

No repository-wide formatter or linter is configured. Avoid unrelated reformatting. Keep audio callbacks allocation-free; schedule browser audio against `AudioContext.currentTime`, not timer arrival times.

## Testing Guidelines

Native tests are custom executables: `synth_smoke` and `rms_measurement`. Web tests use Node's `assert/strict` in `*.test.cjs` files. No numeric coverage threshold exists. Add focused regression checks for changed behavior, especially timing, cancellation, locked notes, RMS accuracy, and state restoration. For UI/audio changes, also verify playback, stop, and responsive layout in a browser.

## Commit & Pull Request Guidelines

History currently contains one descriptive initial commit; no enforced prefix convention exists. Use concise, action-oriented subjects. PRs should explain the behavior change, report relevant test results, link applicable issues, and include screenshots for UI changes.

## Configuration & Deployment

Keep credentials, `.env` files, Wrangler state, and Audacity sessions untracked. `scripts/prepare_orbit_web.py` stages an explicit asset allowlist. `wrangler.orbit.jsonc` targets the live site; deploy only when requested. Consult the Orbit README for deployment commands.
