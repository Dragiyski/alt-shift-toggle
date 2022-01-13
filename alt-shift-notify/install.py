#!/usr/bin/env python3

import argparse
import distutils.file_util
import os
import pathlib
import subprocess
import sys
import venv

if __name__ == '__main__':
    cwd = pathlib.Path(os.path.realpath(os.getcwd()))
    parser = argparse.ArgumentParser('Install the Alt+Shft notify automatically')
    parser.add_argument('install-dir', help='Directory to install to.', type=pathlib.Path, default=cwd)
    args = parser.parse_args()
    cwd = pathlib.Path(os.path.realpath(getattr(args, 'install-dir')))
    sd = pathlib.Path(os.path.realpath(__file__)).parent
    
    cwd.mkdir(parents=True, exist_ok=True)

    print(f'Source Directory: {sd}')
    print(f'Target Directory: {cwd}')

    if cwd != sd:
        distutils.file_util.copy_file(sd.joinpath('main.py'), cwd.joinpath('main.py'), update=1, verbose=1)
        distutils.file_util.copy_file(sd.joinpath('requirements.txt'), cwd.joinpath('requirements.txt'), update=1, verbose=1)

    env_builder = venv.EnvBuilder(clear=True, symlinks=False, system_site_packages=False, with_pip=True)
    env_builder.create(cwd.joinpath('venv'))

    result = subprocess.run([cwd.joinpath('venv', 'bin', 'python'), '-m' 'pip', 'install', '-r', 'requirements.txt'], stdout=sys.stdout, stderr=sys.stderr)
    if result.returncode != 0:
        if result.returncode < 0:
            sys.exit(1)
        sys.exit(result.returncode)

    with open(sd.joinpath('alt-shift-notify.service.in'), mode='r') as file:
        service_script_template = file.read()

    service_script = service_script_template.replace('@INSTALL_DIR@', str(cwd))

    with open(cwd.joinpath('alt-shift-notify.service'), mode='w') as file:
        file.write(service_script)
    distutils.file_util.copy_file(cwd.joinpath('alt-shift-notify.service'), '/etc/systemd/system/alt-shift-notify.service', update=1, link='sym')
