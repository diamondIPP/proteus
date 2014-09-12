#!/usr/bin/env python

"""
Python script parses the source files and automatically generates a Makefile
appropriate for compiling on Linux.
"""

compiler = 'g++'
cflags = '`root-config --cflags` -O2 -Wall'
lflags = '`root-config --ldflags --glibs` -O1'
objpath = 'obj'
srcpath = 'src'
executable = 'Judith'

ext_source = '.cpp'
ext_header = '.h'
sources = []
objects = []

makefile = None 

import os

def find_sources():
    global sources
    global objects

    path = '.'
    if len(srcpath) > 0:
        path = srcpath + '/'

    for root, dir, files in os.walk(path):
        for file in files:
            if len(file) < len(ext_source):
                continue
            if file[-len(ext_source):] != ext_source:
                continue

            source = os.path.join(root[len(path):], file)
            object = file[:-len(ext_source)] + '.o'

            objects.append(object)
            sources.append(source)


def write_preamble():
    makefile.write('CC = ' + compiler + '\n')
    makefile.write('CFLAGS = ' + cflags + '\n')
    makefile.write('LFLAGS = ' + lflags + '\n')
    makefile.write('OBJPATH = ' + objpath + '\n')
    makefile.write('SRCPATH = ' + srcpath + '\n')
    makefile.write('EXECUTABLE = ' + executable + '\n')

    makefile.write('OBJECTS = ')
    for object in objects:
        makefile.write('$(OBJPATH)/' + object + ' ')
    makefile.write('\n\n')


def write_targets():
    makefile.write('all: ' + executable + '\n\n')
    makefile.write(
            executable + ': $(OBJECTS)\n'
            '\t$(CC) $(OBJECTS) $(LFLAGS) -o $(EXECUTABLE)\n\n')

    for (object, source) in zip(objects, sources):
        makefile.write(
                '$(OBJPATH)/' + object + ': $(SRCPATH)/' + source + '\n'
                '\t$(CC) $(CFLAGS) -c $(SRCPATH)/' + source +
                ' -o $(OBJPATH)/' + object + '\n\n')

    makefile.write('clean:\n\t rm $(OBJPATH)/*.o Judith')


if __name__ == "__main__":
    makefile = open('Makefile', 'w')

    find_sources()
    write_preamble()
    write_targets()

    makefile.close()
