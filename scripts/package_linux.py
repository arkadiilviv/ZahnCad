"""Package tested Ubuntu 26.04 binaries with explicit executable modes."""
import pathlib
import shutil
import tarfile

output = pathlib.Path('out/ZahnCAD-Linux-x64')
output.mkdir(parents=True, exist_ok=True)
for name in ('zahncad', 'zahncad-cli'):
    shutil.copy2(pathlib.Path('build-linux') / name, output / name)
for name in ('LICENSE', 'NOTICE'):
    shutil.copy2(name, output / name)
shutil.copy2('build-linux/DEPENDENCIES.txt', output / 'BUILD-DEPENDENCIES.txt')
(output / 'README.txt').write_text(
    'ZahnCAD 0.3.1 research preview — Ubuntu 26.04 x86_64\n\n'
    'Install runtime dependencies:\n'
    'sudo apt-get install libvtk9.5-qt libqt6concurrent6 libqt6sql6-sqlite libarchive13t64\n\n'
    'Run ./zahncad from this directory. An OpenGL-capable desktop is required.\n'
    'B: vertex brush. Left-drag paints; Shift+left erases. Right-drag orbits; Shift+right pans; wheel zooms. F12 lists hotkeys.\n'
    'Includes STL/PLY/OBJ import, selection, cropping, components, controlled repairs, smoothing and simplification.\n'
    'Restorative design and manufacturing validation remain pending.\n'
    'These binaries require matching system libraries; this archive is not an AppImage.\n',
    encoding='utf-8')

def permissions(member):
    member.mode = 0o755 if member.isdir() or pathlib.PurePosixPath(member.name).name in ('zahncad', 'zahncad-cli') else 0o644
    return member

with tarfile.open('out/ZahnCAD-Linux-x64.tar.gz', 'w:gz') as archive:
    archive.add(output, arcname=output.name, filter=permissions)
print('Created out/ZahnCAD-Linux-x64.tar.gz')
