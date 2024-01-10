# **************************************************************************** #
# Project: QtiSAS
# License: GNU GPL Version 3 (see LICENSE)
# Copyright (C) by the authors:
#     2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
# Description: script to prepare a new release
# **************************************************************************** #

import argparse
import datetime
import os
import packaging.version
import re
import subprocess
import sys


qtisas = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
version_file = os.path.join(qtisas, 'qtisas', 'src', 'core', 'globals.h')

def read_current_version():
    version = []
    with open(version_file, 'r') as f:
        line = f.readline()
        while line:
            if 'maj_version' in line:
                version.append(int(line.split(' = ')[1][:-2]))
            if 'min_version' in line:
                version.append(int(line.split(' = ')[1][:-2]))
            if 'patch_version' in line:
                version.append(int(line.split(' = ')[1][:-2]))
            line = f.readline()
    return version


def new_version(version, target=None):
    targets = ['major', 'minor', 'patch']
    if target not in targets:
        print('Cannot determine which version index to update.')
        exit(1)
    i = targets.index(target)
    version[i] = version[i] + 1
    for j in range(i + 1, len(targets)):
        version[j] = 0
    return version


def update_version(file, old, new):
    targets = ['maj_version', 'min_version', 'patch_version']
    fn = os.path.basename(file)
    with open(file, 'r') as f:
        text = f.readlines()
    upd = ''
    str_old = f'{old[0]}.{old[1]}.{old[2]}'
    str_new = f'{new[0]}.{new[1]}.{new[2]}'
    pattern = r'<ReleaseDate>[^<]*</ReleaseDate>'
    if fn == 'globals.h':
        for line in text:
            for target in targets:
                if target in line:
                    i = targets.index(target)
                    line = line.replace(str(old[i]), str(new[i]))
            upd += line
    else:
        for line in text:
            if str_old in line:
                line = line.replace(str_old, str_new)
            if '<ReleaseDate>' in line:
                line = re.sub(pattern, f'<ReleaseDate>{datetime.datetime.now().date()}</ReleaseDate>', line)
            upd += line
    with open(file, 'w') as f:
        f.writelines(upd)
    return


def generate_changelog():
    tags = subprocess.check_output('git tag',
                                   cwd=qtisas, shell=True, text=True)
    tags = tags.split('\n')
    tags = [tag for tag in tags if tag != '']
    tags = sorted(tags, key=packaging.version.parse)
    out = ''
    for i, tag in enumerate(tags):
        log = f'# {tag}\n\n'
        if i == 0:
            commits = subprocess.check_output(f'git log --oneline --format=%s {tag}',
                                              cwd=qtisas, shell=True, text=True)
        else:
            commits = subprocess.check_output(f'git log --oneline --format=%s {tags[i - 1]}..{tag}',
                                              cwd=qtisas, shell=True, text=True)
        for commit in commits.split('\n'):
            if commit:
                log += ' * ' + commit + '\n'
        log += '\n'
        out = log + out
    with open(os.path.join(qtisas, 'changelog.md'), 'w') as f:
        f.write(out)


if __name__ == '__main__':
    # defines files to change
    files_to_update = [
        os.path.join(qtisas, 'CMakeLists.txt'),
        os.path.join(qtisas, 'readme.md'),
        os.path.join(qtisas, 'qtisas', 'src', 'core', 'globals.h'),
        os.path.join(qtisas, 'win', 'config', 'config.xml'),
        os.path.join(qtisas, 'win', 'packages', 'com.qtisas', 'meta', 'package.xml'),
    ]

    # process one of arguments: patch, minor, major
    parser = argparse.ArgumentParser(description='A script to update version indexes in the codebase.')
    parser.add_argument('index', help='Pass "major", "minor" or "patch" to update respective index.')
    if len(sys.argv) > 2:
        print('Too many arguments.')
        exit(1)

    old = read_current_version()
    git = subprocess.check_output('git describe --tags --abbrev=0',
                                  cwd=qtisas, shell=True, text=True).strip('\n')
    if f'v{old[0]}.{old[1]}.{old[2]}' != git:
        print(f'Local version {old} doesn\'t match with the git tag {git}.')
        exit(1)

    # update the files with new indexes
    new = new_version(read_current_version(), sys.argv[1])
    for file in files_to_update:
        print('Updated', file)
        update_version(file, old, new)

    # update changelog
    changelog = f'# v{new[0]}.{new[1]}.{new[2]}\n\n'
    commits = subprocess.check_output(f'git log --oneline --format=%s v{old[0]}.{old[1]}.{old[2]}..HEAD',
                                      cwd=qtisas, shell=True, text=True)
    for commit in commits.split('\n'):
        if commit:
            changelog += ' * ' + commit + '\n'
    changelog += '\n'
    with open(os.path.join(qtisas, 'changelog.md'), 'r') as f:
        log = f.read()
    log = changelog + log
    with open(os.path.join(qtisas, 'changelog.md'), 'w') as f:
        log = f.write(log)

    # commit changes
    files_to_update.append(os.path.join(qtisas, 'changelog.md'))
    for file in files_to_update:
        subprocess.run(f'git add {file}', cwd=qtisas, shell=True)
    version = f'v{new[0]}.{new[1]}.{new[2]}'
    subprocess.run(f'git commit -m "prepare release {version}"',
                   cwd=qtisas, shell=True)
