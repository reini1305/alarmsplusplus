#!/usr/bin/python
import os, sys, string, pdb
import re, fileinput
import ctypes
import struct
import json


#Helper functions
def hash_djb2(string):
    """hash a string using djb2 with seed 5381
    Args:
        string (str): string to hash
    Returns:
        (int): hashval of string masked to force positive by masking 32-bit
    """
    hashval = ctypes.c_uint(5381)
    for char in string:
        hashval.value = ((hashval.value << 5) + hashval.value) + ord(char)
    return hashval.value & 0x7FFFFFFF


def gen_loc_dict(code_dir):
    fileglob_list = []
    loc_dict = {}
    pbllog_regex = "_\(\"(?P<loc>[^\)]+)\"\)"

    for root, dirnames, filenames in os.walk(code_dir):
        for filename in [filename for filename in filenames \
                if ".c" in filename[-2:]]:
            fileglob_list.append(os.path.join(root, filename))
    for filename in fileglob_list:
        with open(filename, 'rb') as afile:
            text = afile.read()
            match_list = re.finditer(pbllog_regex, text)
            if match_list:
                for match in match_list:
                  loc_dict[hash_djb2(match.group('loc'))] = match.group('loc')

    return loc_dict

def main():
    # arguments, print an example of correct usage.
    if len(sys.argv) - 1 != 2:
        print("********************")
        print("Usage suggestion:")
        print("python " + sys.argv[0] + " <code_dir> <loc_english.json>")
        print("********************")
        exit()

    code_dir = sys.argv[1]
    output_filename = sys.argv[2]

    hash_dict = gen_loc_dict(code_dir) 
    json_dict = {str(key) : value for (key, value) in hash_dict.iteritems()}
    json.dump(json_dict, open(output_filename, "wb"), indent=2, sort_keys=True)
    print("%s now has %d entries\n" % (output_filename, len(hash_dict)))
    #create binary resource loadable as a pebble dictionary
    with open(output_filename.replace('.json', '.bin'), 'wb') as output_bin:
      output_bin.write(struct.pack('I', len(hash_dict))) #count of entries
      for (key, value) in hash_dict.iteritems():
        output_bin.write(struct.pack('I',key)) #key
        output_bin.write(struct.pack('I',len(value) + 1)) #length of string including null
        output_bin.write(value) #write string as c string
        output_bin.write(struct.pack('B',0)) #null terminate string


    #output tuple array
    #with open(output_filename + '.bin', 'wb') as output_bin:
      #dump the tuple header for the dictionary
    #  output_bin.write(struct.pack('B', len(hash_dict))) #count of items
    #  for (key, value) in hash_dict.iteritems():
    #    output_bin.write(struct.pack('I',key)) #key
    #    output_bin.write(struct.pack('b',1)) #type cstring
    #    output_bin.write(struct.pack('<H',len(value) + 1)) #length of string including null
    #    output_bin.write(value) #write string as c string
    #    output_bin.write(struct.pack('B',0)) #null terminate string



if __name__ == '__main__':
    main()

