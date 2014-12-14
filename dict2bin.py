#!/usr/bin/python
import os, sys, string, pdb
import re, fileinput
import ctypes
import struct
import json
import sys



def main():
    # arguments, print an example of correct usage.
    if len(sys.argv) - 1 != 1:
        print("********************")
        print("Usage suggestion:")
        print("python " + sys.argv[0] + " <locale_chinese.json>")
        print("********************")
        exit()

    dict_filename = sys.argv[1]

    json_dict = json.load(open(dict_filename, 'rb'))
    hash_dict = {int(key) : value for (key, value) in json_dict.iteritems() if key.isdigit()}
    #create binary resource loadable as a pebble dictionary
    with open(dict_filename.replace('.json','.bin'), 'w') as output_bin:
      output_bin.write(struct.pack('I', len(hash_dict))) #count of entries
      for (key, value) in hash_dict.iteritems():
        output_bin.write(struct.pack('I',key)) #key
        output_bin.write(struct.pack('I',len(value.encode('utf-8')) + 1)) #length of string including null
        output_bin.write(value.encode('utf-8')) #write string as c string
        output_bin.write(struct.pack('B',0)) #null terminate string


if __name__ == '__main__':
    main()

