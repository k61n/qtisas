# **************************************************************************** #
# Project: QtiSAS
# License: GNU GPL Version 3 (see LICENSE)
# Copyright (C) by the authors:
#     2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
# Description: check if clang-tidy warns on the latest pushed changes
# **************************************************************************** #

import gitlab
import os
import subprocess


file = os.path.abspath(__file__)
qtisas_root = os.path.dirname(os.path.dirname(file))


def parse_clangtidy_output(filename):
    cmd = f'clang-tidy -p {os.path.join(qtisas_root, "build")} {filename}'
    output = subprocess.check_output(cmd, shell=True, text=True)
    errors = []
    for line in output.split('\n'):
        if line:
            if line.startswith('/') and 'warning:' in line:
                errors.append({})
                errors[-1]['line'] = line.split(':')[1]
                errors[-1]['warning'] = line
                errors[-1]['note'] = ''
                errors[-1]['fix'] = ''
            elif line.startswith('/') and 'note:' in line:
                errors.append({})
                errors[-1]['line'] = line.split(':')[1]
                errors[-1]['warning'] = ''
                errors[-1]['note'] = line
                errors[-1]['fix'] = ''
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

    def __iter__(self):
        return iter(self._errors.items())

    def __getitem__(self, filename):
        return self._errors[filename]


def get_lines_numbers(lines_to_find, filename):
    numbers = []
    with open(os.path.join(qtisas_root, filename), 'r') as f:
        lines = f.readlines()
    i, current = 0, 0
    for line in lines:
        i += 1
        if line and line[:-1] == lines_to_find[current]:
            current += 1
            if current == len(lines_to_find):
                break
            numbers.append(i)
    return numbers


if __name__ == '__main__':
    # find sha of the latest successful job
    gl = gitlab.Gitlab('https://iffgit.fz-juelich.de/')
    project = gl.projects.get(os.environ.get('CI_PROJECT_ID'))
    commits = project.commits.list(all=True)
    commits_shas = [c.attributes['id'] for c in commits]
    lastjob_sha = None
    for pipeline in project.pipelines.list(iterator=True):
        if pipeline.attributes['ref'] == 'master' and \
                pipeline.attributes['status'] == 'success' and \
                pipeline.attributes['sha'] in commits_shas:
            lastjob_sha = pipeline.attributes['sha']
            break

    # list shas of commits that follow the latest successful job
    commits_to_check = []
    for commit in commits:
        if commit.attributes['id'] == lastjob_sha:
            break
        commits_to_check.append(commit)

    # dict all the affected files
    committed_files = {}
    for commit in commits_to_check:
        for entry in commit.diff():
            if entry['new_path'].split('.')[-1] in ['cpp', 'h'] and \
                    entry['new_path'] not in committed_files.keys():
                committed_files[entry['new_path']] = {'lines': [],
                                                      'numbers': []}

    # exit if there are no cpp or h files
    if not committed_files:
        exit(0)

    # populate corresponding list with each new line number
    for commit in commits_to_check:
        for entry in commit.diff():
            if entry['new_path'] not in committed_files.keys():
                continue
            lines = entry['diff'].split('\n')
            for line in lines:
                if line.startswith('+') and not line.startswith('+++'):
                    committed_files[entry['new_path']]['lines'].append(line[1:])
    for filename in committed_files.keys():
        if committed_files[filename]['lines']:
            committed_files[filename]['numbers'] = \
                get_lines_numbers(committed_files[filename]['lines'], filename)

    # dict clang-tidy warnings for the filenames
    warnings = ClangTidyDB()
    for filename in committed_files.keys():
        print(f'\nClang-tidy on {os.path.join(qtisas_root, filename)}')
        warnings.add_filename(os.path.join(qtisas_root, filename))

    # check for clang-tidy warnings in new lines in filenames
    print('\n\nClang-tidy has detected several warnings. '
          'Please fix these warnings for the pipeline to succeed.\n\n')
    for filename in committed_files.keys():
        entries = warnings.check_filename(os.path.join(qtisas_root, filename),
                                          committed_files[filename]['numbers'])
        if entries:
            for entry in entries:
                if entry["warning"]:
                    print(f'Line {entry["line"]}')
                    print(f'{entry["warning"]}')
                    print(f'Fix:\n{entry["fix"]}')
                # if entry["note"]:
                #     print(f'{entry["note"]}')
                #     print(f'{entry["fix"]}')
            exit(1)
        else:
            exit(0)
