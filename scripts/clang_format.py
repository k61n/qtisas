# **************************************************************************** #
# Project: QtiSAS
# License: GNU GPL Version 3 (see LICENSE)
# Copyright (C) by the authors:
#     2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
# Description: check if clang-format results with corrections for the latest
# pushed changes
# **************************************************************************** #

import os
import re
import subprocess

import gitlab

from fileheader import generate_header


file = os.path.abspath(__file__)
qtisas_root = os.path.dirname(os.path.dirname(file))


def parse_clang_format_output(filenames):
    """Output format {filename: [{'line_nums': [], 'block': ''}}]"""
    cmd_format = lambda filename: f'clang-format -style=Microsoft {filename} ' \
                                  f'> {filename}.formatted'
    cmd_mv = lambda filename: f'mv {filename}.formatted {filename}'
    result = {}
    for filename in filenames:
        subprocess.run(cmd_format(filename), shell=True, cwd=qtisas_root,
                       text=True)
        subprocess.run(cmd_mv(filename), shell=True, cwd=qtisas_root, text=True)
        output = subprocess.check_output(f'git diffn --patience {filename}', shell=True,
                                         cwd=qtisas_root, text=True)
        result[filename] = []
        if output:
            block = {'line_nums': [], 'block': ''}
            new_block = False
            prev_line = ''
            for line in output.split('\n'):
                _line = re.sub(r'\x1b\[.*?m', '', line)
                if not _line or _line[:3] in ['---', '+++'] or '#include' in _line:
                    continue
                if _line.startswith('-') and not new_block:
                    new_block = True
                if _line.startswith('-'):
                    block['line_nums'].append(int(_line.split(':')[0][1:]))
                if _line[0] in ['-', '+'] and new_block:
                    block['block'] += line + '\n'
                if _line[0] not in ['-', '+'] and new_block:
                    if prev_line[0].startswith('+'):
                        new_block = False
                        block['block'] += '\n'
                        result[filename].append(block)
                    block = {'line_nums': [], 'block': ''}
                prev_line = _line
    return result


def get_latest_commits_and_files():
    gl = gitlab.Gitlab('https://iffgit.fz-juelich.de/')
    project = gl.projects.get(os.environ.get('CI_PROJECT_ID'))
    master_commits = project.commits.list(all=True)
    master_shas = [c.attributes['id'] for c in master_commits]
    lastjob_sha = None
    for pipeline in project.pipelines.list(iterator=True):
        if pipeline.attributes['ref'] == 'master' and \
                pipeline.attributes['status'] == 'success' and \
                pipeline.attributes['sha'] in master_shas:
            lastjob_sha = pipeline.attributes['sha']
            break

    latest_shas = master_shas[:master_shas.index(lastjob_sha)]

    filenames = []
    for commit in master_commits:
        if commit.attributes['id'] not in latest_shas:
            break
        for entry in commit.diff(all=True):
            if entry['new_path'].split('.')[-1] in ['cpp', 'h'] and \
                    entry['new_path'] not in filenames:
                filenames.append(entry['new_path'])
    return lastjob_sha, filenames


def get_latest_changes():
    """Output format {filename: [n_of_line,]}"""
    sha, filenames = get_latest_commits_and_files()
    result = {}
    for filename in filenames:
        result[filename] = []
        try:
            output = subprocess.check_output(f'git diffn --patience {sha} HEAD -- {filename}',
                                             shell=True, cwd=qtisas_root, text=True)
        except Exception as e:
            print(f'Exception checking {filename} with diffn:\n\t{e}')
            continue
        if output:
            for line in output.split('\n'):
                _line = re.sub(r'\x1b\[.*?m', '', line)
                if _line.startswith('+') and not _line.startswith('+++') and '/dev/null' not in _line:
                    n = int(_line.split(':')[0][1:])
                    result[filename].append(n)
    return result


if __name__ == '__main__':
    # check the file headers
    status = 0
    for filename in get_latest_commits_and_files()[1]:
        if '3rdparty' not in filename.split('/'):
            if not os.path.exists(filename):
                continue
            header = generate_header(os.path.join(qtisas_root, filename))
            with open(os.path.join(qtisas_root, filename), 'r') as f:
                content = f.read()
            if content[0:len(header)] != header:
                print(f'\tUpdate {filename} with info header to proceed:')
                print(header)
                print()
                status = 1
    if status:
        exit(status)

    # parse clang-format
    latest_changes = get_latest_changes()
    formatted = parse_clang_format_output(latest_changes.keys())
    output_blocks = {}

    for filename in latest_changes.keys():
        output_blocks[filename] = []
        for n in latest_changes[filename]:
            for block in formatted[filename]:
                if n in block['line_nums'] \
                        and block['block'] not in output_blocks[filename]:
                    output_blocks[filename].append(block['block'])

    status = 0
    for filename in output_blocks.keys():
        if output_blocks[filename]:
            status = 1
            print('Clang-format found style mismatches in\n'
                  f'{filename}\n'
                  'related to the latest push. '
                  'Please fix them in order to proceed.')
            for block in output_blocks[filename]:
                print(block)

    exit(status)
