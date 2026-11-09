# Orbit 0.5 · Guide utilisateur

Ouvrez [Orbit web](https://orbit.avalla.dev), puis Écouter. Choisissez la langue en haut : elle reste sur l’appareil et peut changer pendant la lecture, indépendamment des presets.

## Jouer et composer

Choisissez gamme et timbre : Glass, Rubber, Mist, Soft Keys ou Bell. Modifiez les huit notes et verrouillez vos pas préférés. Corps ajoute deux sinusoïdes désaccordées de ±7 cents et une à l’octave inférieure, avec gain normalisé. Essayez 35–60 %. Nouvelles sessions : 35 % ; anciens presets : zéro. Sub et batterie ont leurs volumes propres ; delay et réverbération ajoutent de l’espace.

Flux transforme les phrases ; Capturer conserve notes, silences, durées et batterie. Swing décale les croches alternées. Probabilité choisit les événements ; les notes verrouillées respectent les silences. Énergie est neutre à 50 % ; Fill ajoute une mesure de variation. Enregistrez quatre scènes : leur rappel pendant la lecture attend la prochaine mesure de 4/4. JSON conserve toutes les scènes, sans audio.

## Tempo, fichiers et installation

Réglez BPM sur le web ou le tempo interne dans l’application. Avec Sync DAW, utilisez le tempo et le transport de l’hôte. MIDI exporte synthé, sub et batterie GM, sans effets, Probabilité ni Fill. Enregistrez le mix ou trois WAV stéréo secs et alignés, 16 bits, cinq minutes maximum. Téléchargez avant de fermer ; dans le plugin, terminez l’enregistrement avec son bouton.

[Télécharger pour macOS Intel et Apple Silicon](https://github.com/avalla/luma-orbit/releases/latest/download/Luma-Orbit-macOS-Universal.zip). Copiez VST3 dans `~/Library/Audio/Plug-Ins/VST3/`, AU dans `~/Library/Audio/Plug-Ins/Components/`, puis relancez l’analyse. Signature ad-hoc, sans notarisation. Convertissez les anciens `.orbit` en JSON dans le plugin. Web et plugin partagent les données musicales, avec des moteurs différents. Les paramètres techniques et fenêtres système suivent l’hôte et le système.

Sans son, vérifiez volumes, Probabilité et transport. Sur iPhone, vérifiez volume multimédia et Bluetooth/AirPlay, ouvrez Safari directement et relancez Écouter. Le haut-parleur physique nécessite une validation distincte.
