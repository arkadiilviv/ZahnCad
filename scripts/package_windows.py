"""Bundle the built app and its MinGW DLL dependency closure; no system DLLs copied."""
import argparse
import pathlib
import re
import shutil
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--prefix', type=pathlib.Path, required=True)
parser.add_argument('--build', type=pathlib.Path, default=pathlib.Path('build-desktop'))
parser.add_argument('--output', type=pathlib.Path, default=pathlib.Path('out/ZahnCAD'))
args = parser.parse_args()
prefix, output = args.prefix.resolve(), args.output.resolve()
output.mkdir(parents=True, exist_ok=True)
queue = []
for name in ('zahncad.exe', 'zahncad-cli.exe'):
    queue.append(pathlib.Path(shutil.copy2(args.build / name, output / name)))
for group, plugin in (('platforms', 'qwindows.dll'), ('sqldrivers', 'qsqlite.dll')):
    destination = output / group
    destination.mkdir(exist_ok=True)
    queue.append(pathlib.Path(shutil.copy2(prefix / 'share/qt6/plugins' / group / plugin,
                                          destination / plugin)))

available = {p.name.lower(): p for p in (prefix / 'bin').glob('*.dll')}
seen = {binary.name.lower() for binary in queue}
while queue:
    binary = queue.pop()
    result = subprocess.run([str(prefix / 'bin/objdump.exe'), '-p', str(binary)],
                            check=True, capture_output=True, text=True)
    for name in re.findall(r'DLL Name:\s*(\S+)', result.stdout):
        dependency = available.get(name.lower())
        if dependency and name.lower() not in seen:
            seen.add(name.lower())
            target = output / dependency.name
            shutil.copy2(dependency, target)
            queue.append(target)

(output / 'qt.conf').write_text('[Paths]\nPlugins=.\n', encoding='utf-8')
(output / 'START-HERE.txt').write_text(
    'Run zahncad.exe. Keep this entire folder together.\n'
    'ZahnCAD 0.3.1 research preview: STL/PLY/OBJ viewing and scan preparation.\n'
    'B: vertex brush. Left-drag paints, Shift+left erases. Right-drag orbits, Shift+right pans, wheel zooms.\n'
    'F12 opens all camera/tool hotkeys.\n'
    'Use Prepare scan for selection, cropping, components, repairs, smoothing and simplification.\n'
    'Use Design to select a tooth, trace its margin and build a research crown prototype.\n'
    'Anatomical design and manufacturing validation are pending.\n',
    encoding='utf-8')
for name in ('LICENSE', 'NOTICE'):
    shutil.copy2(name, output / name)
shutil.copytree('examples', output / 'examples', dirs_exist_ok=True)
licenses = prefix / 'share/licenses'
if licenses.exists():
    shutil.copytree(licenses, output / 'third-party-licenses', dirs_exist_ok=True)
print(f'Bundled {len(seen)} native binaries in {output}')
