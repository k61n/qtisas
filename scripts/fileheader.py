
import os
import re
import subprocess
import sys


file = os.path.abspath(__file__)
qtisas_root = os.path.dirname(os.path.dirname(file))

AUTHORS = {
    'gudjon': {'name': 'Gudjon I. Gudjonsson', 'email': 'gudjon@gudjon.org'},
    'ion_vasilief': {'name': 'Ion Vasilief', 'email': 'ion_vasilief@yahoo.fr'},
    'kargo': {'name': 'Alex Kargovsky', 'email': 'kargovsky@yumr.phys.msu.su'},
    'jbaehr': {'name': 'Jonas Bähr', 'email': 'jonas@fs.ei.tum.de'},
    'knut': {'name': 'Knut Franke', 'email': 'knut.franke@gmx.de'},
    'sbesch': {'name': 'Stephen Besch', 'email': ''},
    'thzs': {'name': 'Tilman Hoener zu Siederdissen', 'email': 'thzs@gmx.net'},
    'Vitaliy Pipich': {'name': 'Vitaliy Pipich', 'email': 'v.pipich@gmail.com'},
}


def get_committers(filename):
    output = subprocess.check_output(f'git log --follow --format="%ad %an <%ae>" --date=format:"%Y"  -- {filename}',
                                     shell=True, cwd=qtisas_root, text=True)
    authors = {}
    pattern = r'(\d{4})\s+([\w\s]+)\s+<([^>]+)>'
    for line in output.split('\n'):
        match = re.match(pattern, line)
        if match:
            year = match.group(1).strip()
            an = match.group(2).strip()
            ae = match.group(3).strip()
            if not an in authors.keys():
                authors[an] = {'year': year, 'email': ae}
            else:
                if year < authors[an]['year']:
                    authors[an]['year'] = year
    for author in AUTHORS.keys():
        if author in authors.keys():
            temp = authors[author]
            del authors[author]
            temp['email'] = AUTHORS[author]['email']
            authors[AUTHORS[author]['name']] = temp
    return authors


def get_authors(filename):

    def parse_gitblame(line):
        blame = line[line.find('('):line.find(')')]
        pos = 0
        for i, c in enumerate(blame):
            try:
                int(c)
                pos = i
                break
            except:
                pass
        return blame[1:pos].strip()

    output = subprocess.check_output(f'git blame -w {filename}', shell=True,
                                     cwd=qtisas_root, text=True)
    authors = {}
    for line in output.split('\n'):
        an = parse_gitblame(line)
        if an and an not in authors.keys():
            authors[an] = AUTHORS[an]['name'] if an in AUTHORS.keys() else an
    return authors


def read_description(filename):
    description = ''
    found = False
    with open(filename, 'r') as f:
        content = f.read()
        for line in content.split('\n'):
            if found and '*******' not in line:
                description += '\n' + line
            if found and '*******' in line:
                break
            if 'Description: ' in line:
                description = line.split('Description: ')[1]
                found = True
    return description


def generate_header(filename):
    committers = get_committers(filename)
    authors = get_authors(filename)
    description = read_description(filename)
    authors_list = [[committers[an]['year'], an, committers[an]['email']] for _, an in authors.items()]
    header = f'''/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
'''
    for year, an, ae in sorted(authors_list):
        header += f'    {year} {an} <{ae}>\n'
    header += f'Description: {description}\n'
    header += ' ******************************************************************************/'
    return header


if __name__ == '__main__':
    if len(sys.argv) > 1:
        print(sys.argv[1])
    fn = '/Users/kk/work/qtisas/qtisas_kholostov/qtisas/sans/ascii1d/ascii1d18.cpp'

    print(generate_header(fn))
