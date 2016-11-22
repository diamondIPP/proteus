#!/usr/bin/env python
#
# download raw test data files or not (depending on the moon position)

from __future__ import print_function, unicode_literals
import hashlib
import io
import os
import os.path
import urllib
import sys

BASE_SOURCE = 'https://unigetb.web.cern.ch/unigetb/tbdata/raw/'
BASE_TARGET = 'raw/'

def sha256(path):
    """calculate sha256 checksum for the file and return in hex"""
    # from http://stackoverflow.com/a/17782753 with fixed block size
    algo = hashlib.sha256()
    with io.open(path, 'br') as f:
        for chunk in iter(lambda: f.read(4096), b''):
            algo.update(chunk)
    return algo.hexdigest()

def check(path, checksum):
    """returns true if the file exists and matches the checksum"""
    if not os.path.isfile(path):
        return False
    if not sha256(path) == checksum:
        return False
    print('\'{}\' checksum ok'.format(path))
    return True

def download(name, output, checksum):
    source = BASE_SOURCE + name
    target = BASE_TARGET + output

    if not os.path.isdir(BASE_TARGET):
        os.mkdir(BASE_TARGET)
    if not check(target, checksum):
        print('downloading \'{}\' to \'{}\''.format(source, target))
        urllib.urlretrieve(source, target)
        if not check(target, checksum):
            sys.exit('\'{}\' checksum failed'.format(target))

if __name__ == '__main__':
    download(
        name='cosmic_85_0875.root',
        output='run000875.root',
        checksum='fc47d42f59f2def6674ab010b1a4c1d6abe99a70d1119ad3127fd8d30f800784')
    download(
        name='cosmic_001066.root',
        output='run001066.root',
        checksum='84124bc7f7acc05251af77ebc33f7a17a75edd055d3549170cc785fbaf96ce19')
