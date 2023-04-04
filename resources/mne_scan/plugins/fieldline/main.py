"""This is my super module"""
import sys

for path in sys.path:
    print(path)

print("sys.executable: " + str(sys.executable))

print("hello pepe vamos vamos!!!")

from test import core
core.sayHi("Antonio")
