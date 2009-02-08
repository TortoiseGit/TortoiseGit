# -*- coding: utf-8 -*-
# Copyright (c) 2004 Danilo Segan <danilo@kvota.net>.
#
# This file is part of xml2po.
#
# xml2po is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# xml2po is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with xml2po; if not, write to the Free Software Foundation, Inc.,
# 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

# This implements special instructions for handling DocBook XML documents
# in a better way.
#
#  This means:
#   — better handling of nested complicated tags (i.e. definitions of
#     ignored-tags and final-tags)
#   — support for merging translator-credits back into DocBook articles
#   — support for setting a language
#

# We use "currentXmlMode" class name for all modes
#  -- it might be better to have it named docbookXmlMode, but it will make loading harder;
#     it is also not necessary until we start supporting extracting strings from more
#     than one document type at the same time
#
import re
import libxml2
import os
import md5
import sys

class docbookXmlMode:
    """Class for special handling of DocBook document types.

    It sets lang attribute on article elements, and adds translators
    to articleinfo/copyright."""
    def __init__(self):
        self.lists = ['itemizedlist', 'orderedlist', 'variablelist',
                      'segmentedlist', 'simplelist', 'calloutlist', 'varlistentry' ]
        self.objects = [ 'table', 'figure', 'textobject', 'imageobject', 'mediaobject',
                         'screenshot' ]
        
    def getIgnoredTags(self):
        "Returns array of tags to be ignored."
        return  self.objects + self.lists

    def getFinalTags(self):
        "Returns array of tags to be considered 'final'."
        return ['para', 'formalpara', 'simpara',
                'releaseinfo', 'revnumber', 'title',
                'date', 'term', 'programlisting'] + self.objects + self.lists

    def getSpacePreserveTags(self):
        "Returns array of tags in which spaces are to be preserved."
        return [
            'classsynopsisinfo',
            'computeroutput',
            'funcsynopsisinfo',
            'literallayout',
            'programlisting',
            'screen',
            'synopsis',
            'userinput'
            ]

    def getStringForTranslators(self):
        """Returns string which will be used to credit translators."""
        return "translator-credits"

    def getCommentForTranslators(self):
        """Returns a comment to be added next to string for crediting translators."""
        return """Put one translator per line, in the form of NAME <EMAIL>."""

    def getStringForTranslation(self):
        """Returns translation of 'translation'."""
        return "translator-translation"

    def getCommentForTranslation(self):
        """Returns a string that explains how 'translation' is to be translated."""
        return """Place the translation of 'translation' here."""

    def _find_articleinfo(self, node):
        if node.name == 'articleinfo' or node.name == 'bookinfo':
            return node
        child = node.children
        while child:
            ret = self._find_articleinfo(child)
            if ret:
                return ret
            child = child.next
        return None

    def _find_lastcopyright(self, node):
        if not node.children:
            return None
        last = node.lastChild()
        tmp = last
        while tmp:
            if tmp.name == "copyright":
                last = tmp
                break
            tmp = tmp.prev
        return last

    def _md5_for_file(self, filename):
        hash = md5.new()
        input = open(filename, "rb")
        read = input.read(4096)
        while read:
            hash.update(read)
            read = input.read(4096)
        input.close()
        return hash.hexdigest()

    def _output_images(self, node, msg):
        if node and node.type=='element' and node.name=='imagedata':
            # Use .fileref to construct new message
            attr = node.prop("fileref")
            if attr:
                dir = os.path.dirname(msg.filename)
                fullpath = os.path.join(dir, attr)
                if os.path.exists(fullpath):
                    hash = self._md5_for_file(fullpath)
                else:
                    hash = "THIS FILE DOESN'T EXIST"
                    print >>sys.stderr, "Warning: image file '%s' not found." % fullpath
                    
                msg.outputMessage("@@image: '%s'; md5=%s" % (attr, hash), node.lineNo(),
                                  "When image changes, this message will be marked fuzzy or untranslated for you.\n"+
                                  "It doesn't matter what you translate it to: it's not used at all.")
        elif node and node.children:
            child = node.children
            while child:
                self._output_images(child,msg)
                child = child.next


    def preProcessXml(self, doc, msg):
        """Add additional messages of interest here."""
        root = doc.getRootElement()
        self._output_images(root,msg)

    def postProcessXmlTranslation(self, doc, language, translators, translation):
        """Sets a language and translators in "doc" tree.
        
        "translators" is a string consisted of "Name <email>" pairs
        of each translator, separated by newlines."""

        root = doc.getRootElement()
        # DocBook documents can be something other than article, handle that as well in the future
        while root and root.name != 'article' and root.name != 'book':
            root = root.next
        if root and (root.name == 'article' or root.name == 'book'):
            root.setProp('lang', language)
        else:
            return
        
        if translators == self.getStringForTranslators():
            return
        else:
            # Now, lets find 'articleinfo' (it can be something else, but this goes along with 'article')
            ai = self._find_articleinfo(root)
            if not ai:
                return

            # Now, lets do one translator at a time
            transgroup = libxml2.newNode("authorgroup")
            lines = translators.split("\n")
            for line in lines:
                line = line.strip()
                match = re.match(r"^([^<,]+)\s*(?:<([^>,]+)>)?$", line)
                if match:
                    last = self._find_lastcopyright(ai)
                    copy = libxml2.newNode("othercredit")
                    if last:
                        copy = last.addNextSibling(copy)
                    else:
                        transgroup.addChild(copy)
                        ai.addChild(transgroup)
                    copy.newChild(None, "contrib", translation.encode('utf-8'))
                    if match.group(1) and match.group(2):
                        holder = match.group(1)+"(%s)" % match.group(2)
                    elif match.group(1):
                        holder = match.group(1)
                    elif match.group(2):
                        holder = match.group(2)
                    else:
                        holder = "???"
                    copy.newChild(None, "othername", holder.encode('utf-8'))

# Perform some tests when ran standalone
if __name__ == '__main__':
    test = docbookXmlMode()
    print "Ignored tags       : " + repr(test.getIgnoredTags())
    print "Final tags         : " + repr(test.getFinalTags())
    print "Space-preserve tags: " + repr(test.getSpacePreserveTags())

    print "Credits from string: '%s'" % test.getStringForTranslators()
    print "Explanation for credits:\n\t'%s'" % test.getCommentForTranslators()
    
    print "String for translation: '%s'" % test.getStringForTranslation()
    print "Explanation for translation:\n\t'%s'" % test.getCommentForTranslation()
    
