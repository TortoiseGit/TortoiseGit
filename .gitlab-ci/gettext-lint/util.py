def escape(s):
    return s.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')

import sys

class Output:
    def __init__(self, roottag, output = sys.stdout):
        self.roottag = roottag
        self.rootwritten = 0
        self.current = []
        self.output = output

    def opentag(self, tag, attributes = None, body = None, close = 0):
        if not self.rootwritten:
            self.output.write('<%s>\n' % self.roottag)
            self.rootwritten = 1
        if not close: self.current.append(tag)
        attr = ''
        if attributes:
            for i in attributes.items():
                attr = attr + ' %s="%s"' % (escape(i[0]), escape(i[1]))
        indent = ' ' * (2 * (len(self.current) + close))
        ending = '>\n'
        if body: ending = '>'
        if close and not body: ending = '/>\n'
        self.output.write('%s<%s%s%s' % (indent, tag, attr, ending))
        if body:
            self.output.write(escape(body))
            if close: self.output.write('</%s>\n' % tag)

    def closetag(self):
        self.output.write('%s</%s>\n' % (' ' * 2 * (len(self.current)),
                                         self.current[-1]))
        self.current = self.current[:-1]

    def finish(self):
        if self.rootwritten:
            self.output.write('</%s>\n' % self. roottag)
        else:
            self.output.write('<%s/>\n' % self. roottag)
