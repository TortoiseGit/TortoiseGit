#!/usr/bin/python
# -*- coding: UTF-8 -*-

try:
    import cElementTree as ET
except ImportError:
    import xml.etree.cElementTree as ET

class Glossary:
    def __init__(self, filename):
        f = open(filename,'r')
        self.dict = {}

        # parse the document
        tree = ET.parse(f)

        nodes = tree.findall('word')
        for node in nodes:
            key = node.findtext('original/')
            values = node.findall('translation/term/')
            try:
                a = self.dict[key]
            except KeyError:
                self.dict[key] = []
            for value in values:
                if value.text is not None:
                    self.dict[key].append(value.text)

    def __getitem__(self, k):
        return self.dict[k]

    def __setitem__(self, k, x):
        self.dict[k] = x

    def __delitem__(self,k):
        del self.dict[k]

    def __len__(self):
        return len(self.dict)

