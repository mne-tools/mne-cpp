import sys

def dump(obj):
  for attr in dir(obj):
    print("obj.%s = %r" % (attr, getattr(obj, attr)))


print("hello there!!!")

sys.argv = '/autofs/cluster/fusion/juan/anaconda3/bin/anaconda'

dump(sys)


