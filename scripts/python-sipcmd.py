
from PyQt4 import pyqtconfig


config = pyqtconfig.Configuration()
print(f'{config.sip_bin} -I {config.pyqt_sip_dir} {config.pyqt_sip_flags}')
