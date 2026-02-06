#by complex. -- 2026/2/6
import zipfile
import shutil
import os
try:
    os.remove('zipfile.zip')
except OSError:
    pass
with open('params','r') as f:
    params = f.readlines()
    shutil.copy(params[0],'copy.osz')
    os.rename('copy.osz','copy.zip')
    zipfile.ZipFile('copy.zip').extractall('tmp')
    os.remove('copy.zip')
open('complete','w')