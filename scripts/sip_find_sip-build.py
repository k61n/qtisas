# **************************************************************************** #
# Project: QtiSAS
# License: GNU GPL Version 3 (see LICENSE)
# Copyright (C) by the authors:
#     2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
# Description: Return path to sip-build
# **************************************************************************** #

import os
import platform


def find_sipbuild(start_dir):
    for root, dirs, files in os.walk(start_dir):
        for file in files:
            if file == "sip-build":
                return os.path.join(root, file)
    return None


if platform.system() == 'Darwin' and platform.machine() == 'arm64':
    path = find_sipbuild("/opt")
    if path is not None:
        print(path)
        exit()

path = find_sipbuild("/usr")
if path is not None:
    print(path)
    exit()
