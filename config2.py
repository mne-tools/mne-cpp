import sys
def dump(obj):
  for attr in dir(obj):
    print("obj.%s = %r" % (attr, getattr(obj, attr)))

#sys.argv = '/autofs/cluster/fusion/juan/anaconda3/bin/anaconda'
print(' ===  configuration script  =======================================================')
dump(sys)
print(' ==================================================================================')

print(' ===   system path  ===============================================================')
for p in sys.path:
  print(p)
print(' ==================================================================================')

print(' ===   system argv ===============================================================')
print(sys.argv)
print(' ==================================================================================')
sys.argv = '/autofs/cluster/fusion/juan/anaconda3/bin/python'
print(' ===   system argv ===============================================================')
print(sys.argv)
print(' ==================================================================================')

import logging
import argparse
import queue
import sys
import time
import multiprocessing

multiprocessing.set_executable('autofs/cluster/fusion/juan/anaconda3/bin/python')

