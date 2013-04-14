#!/bin/env python

import sys

if __name__ == '__main__':
    info = sys.version_info

    print "-lpython%d.%d" % (info[0], info[1])
    
