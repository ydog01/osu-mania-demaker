#by complex. -- 2026/2/6
import shutil
import os
try:
    os.remove('copy.osz')
except OSError:
    pass
try:
    os.remove('copy.zip')
except OSError:
    pass
with open('params','r') as f:
    params = f.readlines()
    shutil.make_archive('tmp','zip','tmp')
    os.rename('tmp.zip','tmp.osz')
    shutil.copy('tmp.osz',params[0])
    os.remove('tmp.osz')
    shutil.rmtree('tmp')

os.remove('params')