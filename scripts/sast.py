# **************************************************************************** #
# Project: QtiSAS
# License: GNU GPL Version 3 (see LICENSE)
# Copyright (C) by the authors:
#     2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
# Description: check if clang-tidy warns on the latest pushed changes
# **************************************************************************** #

import os
import subprocess

from clang_format import get_latest_changes


file = os.path.abspath(__file__)
qtisas_root = os.path.dirname(os.path.dirname(file))


def parse_clangtidy_output(filename):
    cmd = f'clang-tidy -p {os.path.join(qtisas_root, "build")} {filename}'

    try:
        output = subprocess.check_output(cmd, shell=True, text=True)
    except subprocess.CalledProcessError as e:
        output = e.output if 'no such file or directory' not in e.output else ''
    errors = []
    for line in output.split('\n'):
        if line:
            if line.startswith('/'):
                errors.append({'line': line.split(':')[1], 'type': '',
                               'description': line, 'fix': '',})
            if 'warning:' in line:
                errors[-1]['type'] = 'warning'
            elif 'note:' in line:
                errors[-1]['type'] = 'note'
            elif 'error:' in line:
                errors[-1]['type'] = 'error'
            else:
                errors[-1]['fix'] += line + '\n'
    return errors


class ClangTidyDB:
    def __init__(self):
        self._errors = {}

    def add_filename(self, filename):
        self._errors[filename] = parse_clangtidy_output(filename)

    def check_filename(self, filename, lines_numbers):
        output = []
        for entry in self._errors[filename]:
            if int(entry['line']) in lines_numbers:
                output.append(entry)
        return output


if __name__ == '__main__':
    # exit if there are no cpp or h files
    latest_changes = get_latest_changes()
    if not latest_changes:
        exit(0)

    # dict clang-tidy warnings for the filenames
    warnings = ClangTidyDB()
    for filename in latest_changes.keys():
        print(f'\nClang-tidy on {os.path.join(qtisas_root, filename)}')
        warnings.add_filename(os.path.join(qtisas_root, filename))

    # check for clang-tidy warnings in new lines in filenames
    status = 0
    for filename in latest_changes.keys():
        entries = warnings.check_filename(os.path.join(qtisas_root, filename),
                                          latest_changes[filename])
        if entries:
            for entry in entries:
                if filename in entry['description'] and entry['type'] in ['error', 'warning']:
                    status = 1
                    print(f'Line {entry["line"]}')
                    print(f'{entry["description"]}')
                    print(f'Fix:\n{entry["fix"]}')

    if status:
        print('\n\nClang-tidy has detected several warnings. '
              'Please fix these warnings for the pipeline to succeed.\n\n')
    exit(status)
