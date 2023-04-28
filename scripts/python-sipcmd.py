# **************************************************************************** #
# Project: QtiSAS
# License: GNU GPL Version 3 (see LICENSE)
# Copyright (C) by the authors:
#     2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
# Description: Auto-detect SIP/PyQt location; dump commandline
# **************************************************************************** #

import sipconfig
from PyQt5.QtCore import PYQT_CONFIGURATION


cfg = sipconfig.Configuration()
print(f'{cfg.sip_bin} -I {cfg.default_sip_dir}/PyQt5 {PYQT_CONFIGURATION["sip_flags"]}')
