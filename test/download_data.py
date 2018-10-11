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
try:
    from urllib.request import urlretrieve
except ImportError:
    from urllib import urlretrieve

BASE_SOURCE = 'https://test-project-proteus.web.cern.ch/test-project-proteus/data/'
BASE_TARGET = 'data/'
# dataset names and corresponding checksum
DATASETS = {
    'unigetel_ebeam012_nparticles01': 'f025b66031a37a58d9fb3fc9d574aac8597f556d2251ce10cbe08bfdabcda611',
    'unigetel_ebeam120_nparticles01_xygamma': 'e404d731805afa139da83ca577dc0341f91799f9e1361c862297cb6abd0b5d57',
    'unigetel_ebeam120_nparticles01': 'edeadc77354748162848c448cbfa12a9026aba769eecb52db85c382f4e239024',
    'unigetel_ebeam120_nparticles02': '9fc48ea74ea1b5aa5711e0689ae42c132ac2f8357c3c2758aa5379162c2d3393',
}

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
    print('\'%s\' checksum ok' % path)
    return True

def download(name, checksum):
    # not sure if path.join works for urls
    source = BASE_SOURCE + name + '.root'
    target = os.path.join(BASE_TARGET, name + '.root')
    if not check(target, checksum):
        print('downloading \'%s\' to \'%s\'' % (source, target))
        urlretrieve(source, target)
        if not check(target, checksum):
            sys.exit('\'%s\' checksum failed' % target)

if __name__ == '__main__':
    if len(sys.argv) < 2:
        datasets = DATASETS
    else:
        datasets = {_: DATASETS[_] for _ in sys.argv[1:]}
    for name, checksum in datasets.items():
        download(name=name, checksum=checksum)
