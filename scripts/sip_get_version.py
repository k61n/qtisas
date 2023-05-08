# **************************************************************************** #
# Project: QtiSAS
# License: GNU GPL Version 3 (see LICENSE)
# Copyright (C) by the authors:
#     2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
# Description: Return sip version
# **************************************************************************** #

try:
    import sipconfig

    cfg = sipconfig.Configuration()
    print(cfg.sip_version)
except:
    import sipbuild

    print(sipbuild.SIP_VERSION)
