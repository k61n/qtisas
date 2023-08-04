# **************************************************************************** #
# Project: QtiSAS
# License: GNU GPL Version 3 (see LICENSE)
# Copyright (C) by the authors:
#     2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
# Description: Return path to sip-build
# **************************************************************************** #

import os
import platform
import sipbuild


def find_sipbuild(path):
    for root, dirs, files in os.walk(path):
        for file in files:
            if file == "sip-build":
                return os.path.join(root, file)


if platform.system() == 'Linux':
    print(find_sipbuild('/usr/bin'))
else:
    sip_path = str(sipbuild).split("'")[3]
    sip_search_dir = os.path.join('/', *sip_path.split('/')[:3])
    print(find_sipbuild(sip_search_dir))
