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
    'unigetel_ebeam012_nparticles01': '86f2c1188378a525299f442daa656990cab1060abade573a4a963222510ee800',
    'unigetel_ebeam120_nparticles01_misalignxyrotz': '8420cff3532838292f5f2109c830112a3d86d53c6a5e575105d72fead61d90c7',
    'unigetel_ebeam120_nparticles01': '4dfbb74cfcd7b94041c25f366cbb28eebe622b9dd0f0b2b4cb4b4e94d062ae20',
    'unigetel_ebeam120_nparticles02': '51efd25f147e47f4faafa09006bf98e91a65d949bc15ef240a439229959527e4',
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
