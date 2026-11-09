# Orbit 0.5 · Benutzerhandbuch

Öffnen Sie [Orbit web](https://orbit.avalla.dev) und starten Sie die Wiedergabe. Die Sprache oben bleibt lokal gespeichert und lässt sich während der Wiedergabe ändern. Presets speichern musikalische Einstellungen unabhängig davon.

## Klang und Komposition

Wählen Sie Tonleiter und Klang: Glass, Rubber, Mist, Soft Keys oder Bell. Bearbeiten und sperren Sie acht Notenschritte. Körper ergänzt zwei Sinusstimmen mit ±7 Cent Verstimmung und eine Sinuskomponente eine Oktave tiefer, mit normalisiertem Pegel. Probieren Sie 35–60 %. Neue Sitzungen beginnen bei 35 %, ältere Presets bei null. Sub und Schlagzeug haben eigene Pegel; Delay und Hall schaffen Raum.

Fluss verändert Phrasen; Einfangen bewahrt Noten, Pausen, Dauern und Schlagzeug. Swing verzögert abwechselnde Achtel. Wahrscheinlichkeit steuert Ereignisse; gesperrte Noten beachten Pausen. Energie ist bei 50 % neutral; Fill ergänzt eine Taktvariation. Speichern Sie vier Szenen. Während der Wiedergabe wechseln sie am nächsten 4/4-Takt. JSON speichert alle Szenen, kein Audio.

## Tempo, Export und Installation

BPM steuert das Webtempo, internes Tempo die App. Mit DAW-Synchronisation gelten Hosttempo und Transport. MIDI exportiert Synth, Sub und GM-Schlagzeug, ohne Effekte, Wahrscheinlichkeit und Fill. Mix oder drei ausgerichtete trockene Stereo-WAV-Spuren aufnehmen: 16 Bit, höchstens fünf Minuten. Vor dem Schließen herunterladen. Im Plugin die Aufnahme über ihren eigenen Knopf beenden.

[Download für macOS Intel und Apple Silicon](https://github.com/avalla/luma-orbit/releases/latest/download/Luma-Orbit-macOS-Universal.zip). VST3 nach `~/Library/Audio/Plug-Ins/VST3/`, AU nach `~/Library/Audio/Plug-Ins/Components/` kopieren und Plugins neu scannen. Ad-hoc signiert, nicht notarisiert. Alte `.orbit`-Dateien im Plugin als JSON speichern. Web und Plugin teilen musikalische Daten, mit verschiedenen Klangerzeugern. Technische Parameternamen und Systemdialoge folgen Host und Betriebssystem.

Bei Stille Pegel, Wahrscheinlichkeit und Transport prüfen. Auf iPhone Medienlautstärke und Bluetooth/AirPlay prüfen, Safari direkt öffnen und erneut starten. Ein physisches iPhone benötigt einen eigenen Lautsprechertest.
