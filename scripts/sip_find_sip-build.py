# **************************************************************************** #
# Project: QtiSAS
# License: GNU GPL Version 3 (see LICENSE)
# Copyright (C) by the authors:
#     2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
# Description: Return path to sip-build
# **************************************************************************** #

import os
import sipbuild


sip_path = str(sipbuild).split("'")[3]
search_dir = os.path.join('/', *sip_path.split('/')[:3])

for root, dirs, files in os.walk(search_dir):
    for file in files:
        if file == "sip-build":
            print(os.path.join(root, file))
            exit()
