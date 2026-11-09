# Orbit 0.5 · Guía de usuario

Abre [Orbit web](https://orbit.avalla.dev) y pulsa Escuchar. Elige el idioma arriba: se guarda en el dispositivo y puede cambiar durante la reproducción. Los presets guardan ajustes musicales independientes.

## Sonido y composición

Elige escala y timbre: Glass, Rubber, Mist, Soft Keys o Bell. Edita ocho notas y bloquea tus pasos favoritos. Cuerpo añade dos sinusoides desafinadas ±7 cents y otra una octava por debajo, con ganancia normalizada. Prueba 35–60 %. Sesiones nuevas: 35 %; presets antiguos: cero. Sub y batería tienen volúmenes propios; delay y reverberación aportan espacio.

Flujo transforma frases; Capturar conserva notas, silencios, duraciones y batería. Swing retrasa corcheas alternas. Probabilidad decide eventos; las notas bloqueadas respetan los silencios. Energía es neutra al 50 %; Fill añade un compás de variación. Guarda cuatro escenas: durante la reproducción cambian en el siguiente compás de 4/4. JSON conserva todas las escenas, sin audio.

## Tempo, exportación e instalación

Ajusta BPM en el navegador o tempo interno en la aplicación. Con Sync DAW, usa el tempo y transporte del anfitrión. MIDI exporta synth, sub y batería GM, sin efectos, Probabilidad ni Fill. Graba mezcla o tres WAV estéreo secos y alineados: 16 bits, máximo cinco minutos. Descarga antes de cerrar. En el plugin termina la grabación con su propio botón.

[Descarga macOS Intel y Apple Silicon](https://github.com/avalla/luma-orbit/releases/latest/download/Luma-Orbit-macOS-Universal.zip). Copia VST3 a `~/Library/Audio/Plug-Ins/VST3/`, AU a `~/Library/Audio/Plug-Ins/Components/` y vuelve a escanear. Firma ad-hoc, sin notarización. Convierte antiguos `.orbit` a JSON en el plugin. Web y plugin comparten datos musicales con motores diferentes. Los parámetros técnicos y diálogos siguen al anfitrión y sistema.

Si no suena, revisa volúmenes, Probabilidad y transporte. En iPhone comprueba volumen multimedia y Bluetooth/AirPlay, abre Safari directamente y pulsa Escuchar de nuevo. El altavoz físico requiere una validación independiente.
