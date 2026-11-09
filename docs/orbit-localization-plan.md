# Orbit localization

## Scope and invariants

Italian, English, French, German and Spanish across Orbit web, native editor and user documentation. Language
selection persists locally, defaults to the supported system/browser language and
falls back to English. Additional languages can be added to the same catalog.

- One editable catalog supplies both engines' UI translations; generated runtime assets are checked in CI.
- Translate static text, dynamic status/errors, accessibility labels, scales and file actions.
- Language is not an audio parameter or a preset field. Switching must preserve playback, recording, phrase, scenes and all parameter IDs.
- Native host automation identifiers and musical timbre names remain stable. OS-owned dialogs follow the OS language.
- Keep 16 px or larger operational text and check long English labels at mobile widths.
- English user documentation links back to the Italian originals. Internal implementation history is not duplicated.

## Validation

Catalog completeness/placeholders, generated-file freshness, locale fallback/storage
failure, browser switching during playback/recording, native UI/parameter preservation,
existing audio and interchange tests, documentation links, universal builds and release.
