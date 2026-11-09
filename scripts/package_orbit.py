"""Package universal native Orbit builds, sources and dependency notices."""
import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile
import zipfile

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--build-dir', default='build-universal')
parser.add_argument('--juce-dir', required=True)
args = parser.parse_args()
build = root / args.build_dir
juce = Path(args.juce_dir)
delivery = root / 'build' / 'Delivery'
delivery.mkdir(parents=True, exist_ok=True)
archive = delivery / 'Luma-Orbit-macOS-Universal.zip'
with tempfile.TemporaryDirectory(dir=delivery) as temporary:
    package = Path(temporary) / archive.stem
    package.mkdir()
    for fmt, suffix in [('VST3', 'vst3'), ('AU', 'component'), ('Standalone', 'app')]:
        name = f'Luma Orbit.{suffix}'
        target = package / fmt / name
        target.parent.mkdir()
        subprocess.run(['ditto', str(build / 'LumaOrbit_artefacts' / 'Release' / fmt / name), str(target)], check=True)
        binary = target / 'Contents' / 'MacOS' / 'Luma Orbit'
        archs = subprocess.check_output(['lipo', '-archs', str(binary)], text=True).split()
        if set(archs) != {'arm64', 'x86_64'}:
            raise RuntimeError(f'Expected a universal binary: {archs}')
        subprocess.run(['codesign', '--force', '--sign', '-', str(target)], check=True)
        subprocess.run(['codesign', '--verify', '--deep', '--strict', str(target)], check=True)
    source = package / 'Sorgenti' / 'Luma'
    source.mkdir(parents=True)
    for name in ['Source', 'Tests', 'scripts', 'docs', 'locales']:
        shutil.copytree(root / name, source / name, ignore=shutil.ignore_patterns('__pycache__'))
    for name in ['CMakeLists.txt', 'README.md', 'LICENSE']:
        shutil.copy2(root / name, source / name)
    shutil.copy2(root / 'LICENSE', package / 'LICENSE')
    shutil.copy2(juce / 'LICENSE.md', package / 'JUCE-LICENSE.md')
    shutil.copy2(root / 'docs' / 'Luma-Orbit.md', package / 'LEGGIMI.md')
    subprocess.run(['tar', '-czf', str(package / 'Sorgenti' / 'JUCE-8.0.13.tar.gz'), '--exclude=.git', '-C', str(juce), '.'], check=True)
    subprocess.run(['ditto', '-c', '-k', '--sequesterRsrc', '--keepParent', str(package), str(archive)], check=True)
with zipfile.ZipFile(archive) as zipped:
    if zipped.testzip() is not None:
        raise RuntimeError('ZIP integrity failed')
    for suffix, fmt in [('vst3', 'VST3'), ('component', 'AU'), ('app', 'Standalone')]:
        zipped.getinfo(f'{archive.stem}/{fmt}/Luma Orbit.{suffix}/Contents/MacOS/Luma Orbit')
    zipped.getinfo(f'{archive.stem}/LICENSE')
print(f'Verified {archive} ({archive.stat().st_size / 1024**2:.1f} MB)')
