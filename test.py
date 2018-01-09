#!/bin/python
#coding=gb2312
import instantlz4
import json
com=instantlz4.compress("lz4")
print com
print instantlz4.decompress(com)

input = u"申德周".encode('unicode-escape')
com=instantlz4.compress(input)
print com
print instantlz4.decompress(com)
