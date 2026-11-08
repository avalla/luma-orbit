from pathlib import Path
import argparse
import shutil
import subprocess
import zipfile

root = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description='Package local macOS plugin builds with matching sources.')
parser.add_argument('--build-dir', default='build')
parser.add_argument('--universal', action='store_true')
parser.add_argument('--juce-dir', default='/tmp/luma-juce-8.0.13')
args = parser.parse_args()
build_dir = (root / args.build_dir).resolve()
juce_dir = Path(args.juce_dir).resolve()
platform = 'Universal' if args.universal else 'AppleSilicon'
expected_archs = {'arm64', 'x86_64'} if args.universal else {'arm64'}
package = root / 'build/Delivery' / f'Luma-Plugins-macOS-{platform}'
package.mkdir(parents=True, exist_ok=True)
for artifact, name in [('Luma_artefacts', 'Luma'), ('LumaRms_artefacts', 'Luma RMS')]:
    for fmt, suffix in [('VST3', 'vst3'), ('AU', 'component')]:
        src = build_dir / artifact / 'Release' / fmt / f'{name}.{suffix}'
        dst = package / fmt / src.name
        dst.parent.mkdir(exist_ok=True)
        subprocess.run(['ditto', str(src), str(dst)], check=True)
        subprocess.run(['codesign', '--force', '--sign', '-', str(dst)], check=True)
        subprocess.run(['codesign', '--verify', '--deep', '--strict', str(dst)], check=True)
        binary = dst / 'Contents/MacOS' / name
        arch = subprocess.check_output(['lipo', '-archs', str(binary)], text=True).strip()
        if set(arch.split()) != expected_archs:
            raise RuntimeError(f'Unexpected architecture: {arch}')
source = package / 'Sorgenti/Luma'
source.mkdir(parents=True, exist_ok=True)
for name in ['Source', 'Tests', 'docs', 'scripts']:
    shutil.copytree(root / name, source / name, dirs_exist_ok=True)
for name in ['CMakeLists.txt', 'README.md', '.gitignore', 'LICENSE']:
    shutil.copy2(root / name, source / name)
shutil.copy2(root / 'Audio/Prova-RMS.mp3', package / 'Prova-RMS.mp3')
shutil.copy2(juce_dir / 'LICENSE.md', package / 'JUCE-LICENSE.md')
shutil.copy2(root / 'LICENSE', package / 'LICENSE')
subprocess.run(['tar', '-czf', str(package / 'Sorgenti/JUCE-8.0.13.tar.gz'),
                '--exclude=.git', '-C', str(juce_dir), '.'], check=True)
instructions = '''LUMA PLUGINS / macOS Apple Silicon (arm64)

CONTENUTO
- Luma: sintetizzatore a 8 voci, sei preset e tastiera del computer.
- Luma RMS: misuratore RMS con finestra mobile e picco campione.
- Formati VST3 e Audio Unit per entrambi i plugin.
- MP3 di prova di 20 secondi, con quattro livelli di volume.
- Sorgenti del progetto e copia esatta di JUCE 8.0.13 con relative licenze.

INSTALLAZIONE
In Finder scegli Vai > Vai alla cartella.
Copia i bundle dalla cartella VST3 in:
  ~/Library/Audio/Plug-Ins/VST3/
Oppure copia i bundle dalla cartella AU in:
  ~/Library/Audio/Plug-Ins/Components/
Crea la cartella se non esiste. Riavvia la DAW o esegui una scansione dei plugin.
Cerca Luma o Luma RMS, produttore Luma Audio.

USO
Luma va su una traccia strumento MIDI. A S D F G H J K suonano i tasti bianchi;
W E T Y U i diesis. Clicca Suona con la tastiera per acquisire il focus.
Luma RMS va inserito come effetto in tempo reale su una traccia audio.
Apri il misuratore e avvia Play. In Audacity usa il pannello Effetti in tempo reale.
Audacity puo ospitare Luma RMS; per il synth MIDI serve una DAW con strumenti.
Il misuratore conserva l'ultima lettura dopo Stop. Prima della prima riproduzione
-inf indica che il plugin non ha ancora misurato campioni audio.

COMPATIBILITA
Richiede un Mac Apple Silicon e una DAW avviata nativamente.
Non e una build per Mac Intel, Windows o Linux.
Build di sviluppo con firma ad-hoc, senza firma Developer ID o notarizzazione.
macOS potrebbe richiedere un controllo di sicurezza sul computer del destinatario.

SORGENTI
In Sorgenti/Luma trovi codice, test e istruzioni di compilazione.
Per usare la dipendenza inclusa, estrai Sorgenti/JUCE-8.0.13.tar.gz in una cartella
JUCE, poi configura il progetto con -DJUCE_SOURCE_DIR=/percorso/JUCE.
La licenza JUCE e le licenze delle sue dipendenze sono incluse nei sorgenti.
Il codice originale del progetto e distribuito con licenza MIT, inclusa in LICENSE
e Sorgenti/Luma/LICENSE. JUCE e le altre dipendenze mantengono le proprie licenze.
'''
if args.universal:
    instructions = instructions.replace('macOS Apple Silicon (arm64)', 'macOS Universal (Intel x86_64 + Apple Silicon arm64)')
    instructions = instructions.replace('Richiede un Mac Apple Silicon e una DAW avviata nativamente.\nNon e una build per Mac Intel, Windows o Linux.',
        'Contiene entrambe le architetture: Intel x86_64 e Apple Silicon arm64.\nIntel: macOS 10.13 o successivo. Apple Silicon: macOS 11 o successivo.\nLa DAW sceglie automaticamente la versione adatta. Non serve scegliere un file diverso.\nNon e una build per Windows o Linux.')
(package / 'LEGGIMI.txt').write_text(instructions, encoding='utf-8')
archive = package.with_suffix('.zip')
subprocess.run(['ditto', '-c', '-k', '--sequesterRsrc', '--keepParent', str(package), str(archive)], check=True)
with zipfile.ZipFile(archive) as zipped:
    bad = zipped.testzip()
    if bad is not None:
        raise RuntimeError(f'Corrupt entry: {bad}')
    for name in ['VST3/Luma.vst3/Contents/MacOS/Luma', 'AU/Luma.component/Contents/MacOS/Luma',
                 'VST3/Luma RMS.vst3/Contents/MacOS/Luma RMS', 'AU/Luma RMS.component/Contents/MacOS/Luma RMS',
                 'LEGGIMI.txt', 'LICENSE', 'Sorgenti/Luma/LICENSE', 'Prova-RMS.mp3', 'Sorgenti/Luma/Source/Rms/Processor.cpp', 'Sorgenti/JUCE-8.0.13.tar.gz']:
        zipped.getinfo(package.name + '/' + name)
print(f'Verified: {archive} ({archive.stat().st_size / 1024**2:.1f} MB)')
