"""This is my super module"""
import sys
from test import core

for path in sys.path:
    print(path)

print("sys.executable: " + str(sys.executable))

print("hello pepe vamos vamos!!!")

core.sayHi("Antonio")

