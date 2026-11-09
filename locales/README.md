# Orbit translations

`orbit.json` is the source of truth for web and native interface translations. Italian source strings are keys; every key needs English, French, German and Spanish values. Preserve named `{placeholders}` exactly. Technical identifiers and preset fields must remain language independent.

Run `python3 scripts/generate_orbit_locales.py` after editing the catalog. Commit both generated files: `prototypes/orbit/translations.js` and `Source/Orbit/Translations.h`. Run the generator with `--check` and `node prototypes/orbit/i18n.test.cjs` before committing.

Language preference belongs to the interface, never the audio engine or preset. Verify long labels at mobile width and switch languages while playing. User guides live under `docs/{en,fr,de,es}/README.md`; Italian is `docs/Orbit-quickstart.md`.
