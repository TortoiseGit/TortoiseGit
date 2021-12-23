#!/usr/bin/env python
#
# Copyright (C) 2010 the TortoiseSVN team
# This file is distributed under the same license as TortoiseSVN
#
# Author: Kevin O. Grover 2010
#

'''
File Utilities.
'''

from __future__ import with_statement

import datetime
import hashlib
import os
import sys
import time

HASHFILE = '.hashes'

#
# Exceptions
#

def cmp(a, b):
    return (a > b) - (a < b) 

class FileutilError(Exception):
    '''
    Base exception of errors in this module.
    '''
    pass


class NoDirectoryError(FileutilError):
    '''
    No Such Directory
    '''
    pass

#
# Classes
#


class FileData(object):
    '''
    File Data for one file
    '''

    def __init__(self, path, name):
        self.name = name
        self.path = path

        # atime = Access Time (inode or file or both?)
        # ctime = Change Inode (rename, links, etc...)
        # mtime = Modify File (e.g. change contents)
        mode, ino, dev, nlink, uid, gid, size, atime, mtime, ctime = \
            os.stat(os.path.join(self.path, self.name))
        self.size = size
        self.mtime = isotime(mtime)
        self.hash = None
        self.cache = []

    def calc_hash(self):
        chash, cmtime, csize, cname = self.getcachedhash()

        HASHBLOCKSIZE = 10 * 1024

        # TODO: Use chash (rather than just assuming md5)
        # TODO: switch over to sha1 as a default hash function
        if (csize != str(self.size)) or (cmtime != self.mtime):
            fullpath = os.path.join(self.path, self.name)
            # TODO: Make a little file_hash function that accepts a fpobj
            hash = hashlib.md5()
            with open(fullpath, 'rb') as fp:
                while True:
                    data = fp.read(HASHBLOCKSIZE)
                    if not data:
                        break
                    hash.update(data)
            self.hash = hash.hexdigest()
            self.hashtype = 'md5'

            self.cache = [rec for rec in self.cache if rec[3] != self.name]
            self.cache.append((self.hash, self.mtime, self.size, self.name))
            self.writecachedhash()
        else:
            self.hash = chash

    def getcachedhash(self):
        hashfile = os.path.join(self.path, HASHFILE)
        if not os.path.exists(hashfile):
            return None, None, None, None

        cache = []
        lineno = 0
        for line in open(hashfile, 'r').readlines():
            lineno += 1
            res = line.strip().split('\t')
            if len(res) == 4:
                cache.append(res)
            else:
                # TODO: Log?
                print >> sys.stderr, \
                    "%s:%s: Bad hash line, ignored." % (hashfile, lineno)

        self.cache = cache
        srch = [r for r in cache if r[3] == self.name]
        srch = srch[0] if srch else (None, None, None, None)
        return srch

    def writecachedhash(self):
        hashfile = os.path.join(self.path, HASHFILE)
        self.cache.sort(lambda x, y: cmp(x[0], y[0]))
        f = open(hashfile, 'w')
        for r in self.cache:
            if os.path.exists(os.path.join(self.path, r[3])):
                f.write('\t'.join(map(str, r)) + '\n')
        f.close()


class HashRecord(object):
    '''
    Hash Record for one file.
    '''

    # Separator used for record data items (on input/output)
    SEP = '\t'

    def __init__(self, data=None):
        if data:
            self.parse(data)
        else:
            self.filename = None
            self.hashcode = None
            self.datetime = None
            self.filesize = None

    def parse(self, data):
        if data[-1] == '\n':
            data = data[:-1]

        hc, dt, sz, fn = data.split(self.SEP, 4)

        dt = datetime.datetime.strptime(dt, '%Y-%m-%dT%H:%M:%S')

        self.filename = fn
        type, sep, code = hc.rpartition(':')
        self.hashcode = code
        self.hashtype = (type if type else 'md5')
        self.datetime = dt
        self.filesize = int(sz)

    def __repr__(self):
        return self.SEP.join(("%s:%s" % (self.hashtype, self.hashcode),
                              self.datetime.strftime('%Y-%m-%dT%H:%M:%S'),
                              str(self.filesize),
                              self.filename))


class HashDir(object):
    '''
    Manages file hash information for a Directory.

    The format of the file is:
    # Comment lines
    hashcode<SEP>date/time<SEP>size<SEP>filename

    Where date time is in ISO format YYYY-MM-DDTHH:MM:SS
    T=Time Marker
    '''

    def __init__(self, dir):
        if not os.path.isdir(dir):
            raise IOError("No such directory: '%s'" % (dir, ))
        self.hashfile = os.path.join(dir, HASHFILE)
        self.clear()
        self.readfile()

    def clear(self):
        '''
        Erase all stored data
        '''
        self.hashes = {}
        self.dirty = False

    def readfile(self):
        '''
        Read the Hash File data from Disk
        '''

        if os.path.exists(self.hashfile):
            self.parse(open(self.hashfile, 'r').readlines())

    def writefile(self):
        '''
        Write out hashes file (if it has changed).
        '''
        if self.dirty:
            fp = open(self.hashfile, 'w')
            fp.write(str(self))
            fp.close()
            self.dirty = False

    def parse(self, hashdata):
        '''
        Parse the contents of the file into a hashed array
        '''

        hashes = {}
        for line in hashdata:
            hr = HashRecord(line)
            hashes[hr.filename] = hr

        self.hashes = hashes

    def getfile(self, name):
        '''
        Look up a file and return it's HashRecord (or None)
        '''
        if name in self.hashes:
            return self.hashes[name]
        else:
            return None

    def addfile(self, name):
        '''
        Add (or Update) a file's HashRecord
        '''
        raise NotImplemented        # TODO: Need to add!
        self.dirty = True

    def __str__(self):
        '''
        String representation of the HashDir.
        This is the same format as the actual file and is used
        to re-create the files when they are updated.
        '''

        if self.hashes:
            hashes = self.hashes.values()
            hashes.sort(lambda x, y: cmp(x.hashcode, y.hashcode))
            data = [str(hr) for hr in hashes]
            return os.linesep.join(data)
        else:
            return ''


class FileWalker(object):
    '''
    Walk a directory performing processing each file and directory.
    '''

    # List of Valid file extensions (None = All are OK)
    VALIDEXT = None

    # Directories to Exclude from processing (primarily revision control)
    EXCLUDEDIRS = '.svn _svn CVS RCS .hg .git'.split()

    def __init__(self, dir):
        if not os.path.isdir(dir):
            raise NoDirectoryError("Directory does not exist: '%s'" % (dir))
        self.dir = dir

    def walk(self):
        #self.__processdir(unicode(self.dir))
        self.__processdir(self.dir)

    def __processdir(self, directory):
        for dirpath, dirnames, filenames in os.walk(directory):
            for dir in dirnames:
                if dir in self.EXCLUDEDIRS:
                    dirnames.remove(dir)
                else:
                    self.onProcessDir(dirpath, dir)
            self.__processfiles(dirpath, filenames)

    def __processfiles(self, dirpath, files):
        for file in files:
            if self.VALIDEXT and (fileext(file) not in self.VALIDEXT):
                # If we have a list of valid extensions, and this
                # file is NOT in it, then continue with the next directory
                # i.e. do NOT process this file.
                continue
            self.onProcessFile(dirpath, file)

    def onProcessDir(self, dirpath, dir):
        '''
        Called when a directory is encountered.
        Should be over riden.
        '''
        pass

    def onProcessFile(self, dirpath, file):
        '''
        Called when a file is encountered.
        Should be over riden.
        '''
        pass


class SimpleFileWalker(FileWalker):
    '''
    A Simple Walker that just prints each dir and file.
    Mostly for testing.
    '''

    def onProcessDir(self, dirpath, dir):
        print ("DIR: <%r> <%r>" % (dirpath, dir))

    def onProcessFile(self, dirpath, file):
        print ("FILE: <%r> <%r>" % (dirpath, file))

# ---------------------------------------------------------
#  Utility Functions
# ---------------------------------------------------------


def fileext(file):
    '''Return the files extension.'''
    return os.path.splitext(file)[1]


def isotime(timestamp):
    '''Return a time as an ISO formatted string.'''
    t = time.localtime(timestamp)
    return time.strftime("%Y-%m-%dT%H:%M:%S", t)


def __test_walk(dirs):
    for dir in dirs:
        print ('SimpleFileWalker(%r):' % (dir, ))
        w = SimpleFileWalker(dir)
        w.walk()


if __name__ == '__main__':
    __test_walk(sys.argv[1:])
