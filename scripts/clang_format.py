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
    """Output format {filename: [{'line_nums': [], 'block': ''}]}"""
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
    # searching for the latest commit with skipped .pre stage
    lastskip_sha = None
    for commit in master_commits:
        if '.pre=skip' in commit.attributes['title'] or \
                '.pre=skip' in commit.attributes['message']:
            lastskip_sha = commit.attributes['id']
            break
    # searching for the latest successfully job that is also on the master
    lastjob_sha = None
    for pipeline in project.pipelines.list(iterator=True):
        if pipeline.attributes['ref'] == 'master' and \
                pipeline.attributes['status'] == 'success' and \
                pipeline.attributes['sha'] in master_shas:
            lastjob_sha = pipeline.attributes['sha']
            break
    # which one is the most recent
    last_sha = None
    for c in master_commits:
        if lastskip_sha == c.attributes['id']:
            last_sha = lastskip_sha
            break
        if lastjob_sha == c.attributes['id']:
            last_sha = lastjob_sha
            break

    if not last_sha:
        return None, []

    filenames = []
    for commit in master_commits:
        if commit.attributes['id'] == last_sha:
            break
        for entry in commit.diff(all=True):
            if entry['new_path'].split('.')[-1] in ['cpp', 'h'] and \
                    entry['new_path'] not in filenames:
                filenames.append(entry['new_path'])
    return last_sha, filenames


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


class Buffer:
    def __init__(self, n):
        self._n = n
        self._arr = []

    def append(self, value):
        if len(self._arr) < self._n:
            self._arr.append(value)
        else:
            self._arr = self._arr[1:] + [value]

    def __len__(self):
        return len(self._arr)

    def __repr__(self):
        return self._arr.__repr__()

    def __iter__(self):
        return self._arr.__iter__()


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

    # parses clang-format
    latest_changes = get_latest_changes()
    formatted = parse_clang_format_output(latest_changes.keys())
    chunks_changed = {}
    chunks_formatted = {}
    expressions = {}
    blocks = {}

    # sees if changed lines appear in clang-format output
    for fn in latest_changes.keys():
        # groups subsequently changed lines into chunks
        chunks_nums = []
        row_nums = sorted(latest_changes[fn])
        for i, n in enumerate(row_nums):
            if i == 0 or row_nums[i - 1] != n - 1:
                chunks_nums.append([n])
            else:
                chunks_nums[-1].append(n)
        # appends changed chunks to a list
        # and strips changed chunks from whitespaces for comparison
        # appends corresponding formatted block for further analysis
        chunks_changed[fn] = []
        chunks_formatted[fn] = []
        expressions[fn] = []
        blocks[fn] = []
        for chunk in chunks_nums:
            chunks_changed[fn].append('')
            chunks_formatted[fn].append('')
            expressions[fn].append('')
            blocks[fn].append('')
            for n in chunk:
                l = len(formatted[fn])
                for i, block in enumerate(formatted[fn]):
                    if n in block['line_nums'] and block['block'] not in chunks_changed[fn]:
                        # sometimes git diff fails due to single whitespace change
                        # just in case add prev and next blocks
                        # strips color ANSI escape codes
                        bl = re.sub(r'\x1b\[.*?m', '', formatted[fn][i - 1]['block']) if i else ''
                        bl += re.sub(r'\x1b\[.*?m', '', block['block'])
                        bl += re.sub(r'\x1b\[.*?m', '', formatted[fn][i + 1]['block']) if i < l - 1 else ''
                        # has to match `-  45      :-    if (example_code)`
                        match = re.compile(rf'-\s*?{n}\s*:-(.*)').search(bl)
                        chunks_changed[fn][-1] += match.group(0) + '\n'
                        expressions[fn][-1] += re.sub(r'\s*', '', match.group(1))
                        blocks[fn][-1] = re.sub(r'-\s*?\d+.*?\n', '', bl)
            if not blocks[fn][-1]:
                continue

            # parses formatted block to match with expression
            lines = blocks[fn][-1].split('\n')
            for i in range(len(lines)):
                buff = Buffer(i + 1)
                for line in lines:
                    match = re.compile(rf'\+\s*\d+:\+(.*)').search(line)
                    _line = '' if not match else match.group(1)
                    if not _line:
                        continue
                    buff.append(_line)
                    if len(buff) == i + 1:
                        if re.sub(r'\s*', '', ''.join(buff)) == expressions[fn][-1]:
                            chunks_formatted[fn][-1] += '\n'.join(buff) + '\n'

    # check if there are format mistmatches
    status = {}
    for fn in chunks_changed.keys():
        status[fn] = 0
        for chunk in chunks_changed[fn]:
            if chunk:
                status[fn] = 1
                break
    # generates final output
    for fn in chunks_changed.keys():
        if status[fn]:
            print(f'{fn}\n')
            for changed, formatted in zip(chunks_changed[fn], chunks_formatted[fn]):
                if changed:
                    print('\033[31m' + re.sub(r'\s*?:-.*?\n', '', changed).strip(' ').replace('-', '\n') + '\033[0m')
                    print('\033[31m' + re.sub(r'-\s*?\d+\s*?:-', '', changed) + '\033[0m')
                if formatted:
                    print('\033[32m' + formatted + '\033[0m')

    if 1 in status.values():
        print('''
Clang-format found style mismatches related to the latest push.
Please fix them in order to proceed.
When you see duplicates, means code fragment repeats.
Sometimes suggestions might not be present due to `git diff` fail.
''')
        exit(1)
    else:
        exit(0)
