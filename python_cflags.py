#!/bin/env python

import distutils.sysconfig
import sys

if __name__ == '__main__':
    xlinker = distutils.sysconfig.get_config_var('LINKFORSHARED')
    info = sys.version_info
    
    print "%s -I/usr/include/python%d.%d" % (xlinker, info[0], info[1])

    
