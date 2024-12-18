# -*- coding: utf-8 -*-

# PO file classes
#
# Pedro Morais <morais@kde.org>
# José Nuno Pires <jncp@netcabo.pt>
# João Miguel Neves <joao@silvaneves.org>
# Sven Strickroth <email@cs-ware.de>
# (c) Copyright 2003, 2004, 2024
# Distributable under the terms of the GPL - see COPYING

import string
import capitalization

class POFile:

    def __init__(self, filename):
        self.filename = filename
        self.fuzzy = None
        self.untranslated = None
        self.translated = None
        self.validateError = None
        self.data = []
        self.errors = []
        self.spellErrors = []
        self.glossaryErrors = []
        self.ignoreConsistency = []
        self.consistencyAlias = []
        self.allowCount = None
        self.enableCheckAccelerator = 1
        self.enableCheckCapitalization = 0
        self.accelerator = '&'
        self.enableCheckEndPontuation = 1
        self.enableCheckLineLength = 0
        self.ignoreFuzzy = 0
        self.spellExtra = []
        self.spellCommand = 'cat'

    def getRatio(self):
        if self.translated is None: return None
        total = self.translated
        if self.fuzzy: total = total + self.fuzzy
        if self.untranslated: total = total + self.untranslated
        return float(self.translated) / float(total)

    def isFullyTranslated(self):
        return self.translated and not(self.fuzzy) and not(self.untranslated)

    def validate(self):
        import subprocess
        command = "msgfmt --statistics -o /dev/null %s 2>&1" % self.filename
        process = subprocess.Popen(command, shell = True, stdout = subprocess.PIPE, stderr = subprocess.PIPE, text = True)
        output, _ = process.communicate()
        self.translated = self.__vextract(output, " translated")
        if self.translated is None: self.validateError = output
        self.fuzzy = self.__vextract(output, " fuzzy")
        self.untranslated = self.__vextract(output, " untranslated")
        return self.validateError is None

    def __vextract(self, output, token):
        end = output.find(token)
        if end == -1: return None
        start = output.rfind(" ", 0, end) + 1
        return int(output[start:end])

    def read_lines(self):
        try:
            pofile = open(self.filename)
            lines = pofile.readlines()
            pofile.close()
            return lines
        except IOError:
            return None

    def parse(self):
        lines = self.read_lines()
        if lines is None: return 0
        current = ""
        msgid = ""
        mode = 0
        self.data = []
        line = 0
        msgidline = 0
        message = 0
        mode1fuzzy = 0
        fuzzy = 0
        for i in lines:
            line = line + 1
            l = i.strip()
            if len(l) == 0: continue
            # msgctx lines are tips for the translator
            if i[:6] == "msgctx": continue
            if i[0] == '#':
                if i.startswith('#, fuzzy'): fuzzy = 1
                continue
            if i[:6] == "msgid " or i[:6] == "msgid\t":
                if mode == 2:
                    self.data.append((msgidline, message, msgid, current,
                                      mode1fuzzy))
                current = ""
                mode = 1
                msgidline = line
                message = message + 1
                mode1fuzzy = fuzzy
                fuzzy = 0
                l = l[6:]
            if i[:7] == "msgstr " or i[:7] == "msgstr\t":
                if mode == 1: msgid = current
                current = ""
                mode = 2
                l = l[7:]
            if mode: current = current + l[1:-1]
        if mode == 2:
            self.data.append((msgidline, message, msgid, current, mode1fuzzy))
        return 1

    def replace(self, number, text, removeFuzzy, output):
        prepare = self.prepare_replace(number)
        if prepare is None: return 0
        if output is not None:
            self.execute_replace(prepare, text, removeFuzzy, output)
        return 1

    def prepare_replace(self, number):
        lines = self.read_lines()
        if lines is None: return None

        line = 0
        message = 0
        headline = None
        fuzzyline = None
        fuzzylinetmp = None

        for i in lines:
            line = line + 1
            l = i.strip()
            if len(l) == 0 or i[0] == '#':
                if i.startswith('#, fuzzy'): fuzzylinetmp = line
                if headline:
                    line = line - 1
                    break
            elif i[:6] == "msgid " or i[:6] == "msgid\t":
                message = message + 1
                fuzzyline = fuzzylinetmp
                fuzzylinetmp = None
            elif i[:7] == "msgstr " or i[:7] == "msgstr\t":
                if message == number: headline = line
        if not(headline): return None
        return (headline, line + 1, lines, fuzzyline)

    def execute_replace(self, prepare, text, removeFuzzy, output,
                        breaknewlines = 0):
        headline, tailline, lines, fuzzyline = prepare
        line = 0
        for i in lines[:headline - 1]:
            line = line + 1
            if removeFuzzy and fuzzyline is not None and fuzzyline == line:
                i = i.replace(', fuzzy', '')
                if i != '#\n': output.write(i)
            else:
                output.write(i)
        for i in self.consistencyAlias:
            text = text.replace(i[1], i[0])
        if breaknewlines and text.find('\\n') >= 0:
            output.write('msgstr ""\n')
            for i in text.split('\\n'):
                if len(i): output.write('"%s\\n"\n' % i)
        else:
            output.write('msgstr "%s"\n' % text)
        for i in lines[tailline - 1:]: output.write(i)

    def append_header_string(self, line, key, list):
        x = self.get_header_string(line, key)
        if x is not None: list.append(x)

    def append_header_strings(self, line, key, list):
        x = self.get_header_string(line, key)
        if x is not None:
            for i in x.split(" "): list.append(i)

    def get_header_string(self, line, key, current = None):
        if line.startswith("X-POFile-%s: " % key):
            return line[len("X-POFile-%s: " % key):]
        return current

    def get_header_int(self, line, key, current = None):
        x = self.get_header_string(line, key)
        if x is None: return current
        try:
            return int(x)
        except:
            return current

    def parseHeader(self):
        if len(self.data) > 0:
            headerLines = self.data[0][3].split('\\n')
            for i in headerLines:
                self.allowCount = self.get_header_int(
                    i, "Allow", self.allowCount)
                self.enableCheckAccelerator = self.get_header_int(
                    i, "CheckAccelerator", self.enableCheckAccelerator)
                self.enableCheckEndPontuation = self.get_header_int(
                    i, "CheckEndPontuation", self.enableCheckEndPontuation)
                self.enableCheckLineLength = self.get_header_int(
                    i, "CheckLineLength", self.enableCheckLineLength)
                self.append_header_string(i, "IgnoreConsistency",
                                          self.ignoreConsistency)
                self.append_header_strings(i, "SpellExtra",
                                           self.spellExtra)
                if i.startswith("X-POFile-ConsistencyAlias: "):
                    s = i[len("X-POFile-ConsistencyAlias: "):].split(" ")
                    if len(s) == 2: self.consistencyAlias.append(s)

    def toWordList(self, s):
        s = s.replace('\\n', ' ').replace('\\t', ' ').replace(',', ' ').replace('.', ' ').replace('?', ' ').replace('!', ' ').replace(':', ' ').replace(';', ' ').replace('(', ' ').replace(')', ' ').replace('/', ' ').replace('&&', ' ').replace('&', '').replace('</', ' ').replace('<', ' ').replace('>', ' ').replace('\\"', " ")
        l = s.split()
        if l is None or len(l) < 1: return l
        ret = {}
        for i in l: ret[i.lower()] = i.lower()
        return ret.values()

    def findEquiv(self, minLength, equiv):
        for l, m, i, s, fuzzy in self.data:
            if len(i) == 0 or len(s) == 0: continue
            si = self.toWordList(i)
            if len(si) == 0: continue
            ss = self.toWordList(s)
            if len(ss) == 0: continue

            for word in si:
                if self.trueStringLen(word) > minLength:
                    countMap = {}
                    if word in equiv: countMap = equiv[word]
                    for corr in ss:
                        if self.trueStringLen(corr) > minLength:
                            count = 0
                            if corr in countMap: count = countMap[corr]
                            countMap[corr] = count + 1
                    equiv[word] = countMap
        return equiv

    def __read_lines_thread(self, child_out):
        self.read_lines_buffer = child_out.readlines()
        return 0

    def spell(self, dict = None):
        import subprocess
        from threading import Thread
        if dict is None:
            dict = {}
        process = subprocess.Popen(self.spellCommand, stdin = subprocess.PIPE, stdout = subprocess.PIPE, stderr = subprocess.PIPE, text = True, shell = True)
        read_lines_thread = Thread(target=self.__read_lines_thread,
                                   args=(process.stdout,));
        read_lines_thread.start()
        x = self.getCleanMsgstr()
        process.stdin.write(x)
        process.stdin.close()
        read_lines_thread.join()
        words = self.read_lines_buffer
        process.stdout.close()
        wse = dict.copy()
        for word in self.spellExtra: wse[word] = word
        ws = {}
        for word in words:
            word = word.strip()
            if word not in wse: ws[word] = word
        self.spellErrors = list(ws.keys())
        return 1

    def glossary(self, glossary):
        self.glossaryErrors = []
        msg = 0
        for line, message, msgid, msgstr, fuzzy in self.data:
            msg = msg + 1
            words = msgid.split(' ') # Should cover more cases than this.
            for word in words:
                error = 0 # If it's not on the glossary, then it's not wrong.
                try:
                    lword = word.lower()
                    if (glossary[lword]):
                        error = 1
                        translations = glossary[lword]
                        for translation in translations:
                            if (msgstr.decode('utf-8').lower().find(translation.lower()) > -1):
                                error = 0
                                break
                except KeyError:
                    pass
                if (error == 1):
                    self.glossaryErrors.append((line, msg, word))
        return 1

    def consistency(self, map, strip):
        if self.parse() == 0: return 0
        self.parseHeader()
        for line, message, msgid, msgstr, fuzzy in self.data:
            if self.ignoreFuzzy and fuzzy: msgstr = ''
            if len(msgid) > 0 and not(msgid in self.ignoreConsistency):
                for i in strip:
                    msgstr = msgstr.replace(i, '')
                    msgid = msgid.replace(i, '')
                for i in self.consistencyAlias:
                    msgstr = msgstr.replace(i[0], i[1])
                    msgid = msgid.replace(i[0], i[1])
                cmsgid = map.get(msgid)
                add = (self.filename, message)
                if cmsgid is None:
                    map[msgid] = { msgstr: [add,] }
                else:
                    cmsgstr = cmsgid.get(msgstr)
                    if cmsgstr is None:
                        cmsgid[msgstr] = [add,]
                    else:
                        cmsgstr.append(add)
        return 1

    def check(self):
        self.errors = []
        msg = 0
        for l, m, i, s, fuzzy in self.data:
            msg = msg + 1
            if len(i) == 0 or len(s) == 0: continue
            if fuzzy and self.ignoreFuzzy: continue
            if self.enableCheckAccelerator:
                self.checkAccelerator(i, s, l, msg, self.accelerator)
            if self.enableCheckCapitalization:
                self.checkCapitalization(i, s, l, msg)
            if self.enableCheckEndPontuation:
                self.checkEndPunctuation(i, s, l, msg)
            if len(s) > 1 and s[:2] == "_:":
                self.errors.append((l, msg, '_: on msgstr'))
            if s[-1] == "<" and i[-1] != '<':
                self.errors.append((l, msg, '< trailling msgstr'))
            if i.count('\\n') == 0 and s.count('\\n') > 0:
                self.errors.append((l, msg, 'extra \\n'))
            if self.enableCheckLineLength:
                self.checkLineLength(s, l, msg, self.enableCheckLineLength,
                                     single_lines = 1)

    def checkAccelerator(self, i, s, pos, msg, acc):
        if i.count(acc) == 1 and i.count(acc + ' ') == 0 and s.count(acc) == 0:
            self.errors.append((pos, msg, 'missing %s acelerator' % acc))
        if i.count(acc) == 1 and i.count(acc + ' ') == 0 and s.count(acc) > 1:
            self.errors.append((pos, msg, 'extra %s acelerator' % acc))
        if i.count(acc) == 0 and s.count(acc) == 1:
            self.errors.append((pos, msg, 'extra %s acelerator' % acc))

    def checkCapitalization(self, i, s, pos, msg):
        if i.startswith("_: NAME OF TRANSLATORS"): return
        if i.startswith("_: EMAIL OF TRANSLATORS"): return
        ic = capitalization.capitalization(i, (), ('to',))
        if ic != capitalization.CAP_UNKNOWN:
            sc = capitalization.capitalization(s, (), ('de', 'do',
                                                       'da', 'dos',
                                                       'das', 'o', 'a',
                                                       'e', 'os', 'as',
                                                       'pelo', 'para'))
            if sc != ic:
                self.errors.append((pos, msg, 'capitalization error'))

    def checkEndPunctuation(self, i, s, pos, msg):
        if i[-1] == ':' and s[-1] != ':':
            self.errors.append((pos, msg, 'missing :'))
        if len(i) > 1 and len(s) > 1 and i[-2:] == ": " and s[-2:] != ": ":
            self.errors.append((pos, msg, 'missing : '))
        if len(i) > 2 and len(s) > 2 and i[-3:] == "..." and s[-3:] != "...":
            self.errors.append((pos, msg, 'missing ...'))
        elif i[-1] == '.' and s[-1] != '.':
            self.errors.append((pos, msg, 'missing .'))

    def checkLineLength(self, line, pos, msg, limit, single_lines):
        linesplit = line.split('\\n')
        if len(linesplit) > 1:
            il = 0
            for f in linesplit:
                il = il + 1
                if len(f) > limit:
                    lenf = self.trueStringLen(f)
                    if lenf > limit:
                        self.errors.append((pos, msg, '%d > %d chars [line %d]'
                                            % (lenf, limit, il)))
        elif single_lines:
            f = linesplit[0]
            if len(f) > limit:
                lenf = self.trueStringLen(f)
                if lenf > limit:
                    self.errors.append((pos, msg, '%d > %d chars'
                                        % (lenf, limit)))

    def trueStringLen(self, f):
        unicode = 0
        for i in f:
            if ord(i) > 127: unicode = unicode + 1
        return len(f) - unicode / 2

    def getMsgstr(self):
        msgstr = ''
        for l, m, i, s, fuzzy in self.data:
            if i != "": msgstr = msgstr + s + '\n'
        return msgstr

    def searchWordInText(self, wordToFind, textToSearch, context, index):
        x = textToSearch.find(wordToFind, index)
        if x < 0: return (x, None)
        lwf = len(wordToFind)
        lts = len(textToSearch)
        if x > 0 and textToSearch[x - 1] in string.letters:
            return (x, None)
        if x < lts - lwf - 1 and textToSearch[x + lwf] in string.letters:
            return (x, None)
        xa, ea = x - context, '...'
        if xa < 0: xa, ea = 0, ''
        xb, eb = x + lwf + context, '...'
        if xb > lts: xb, eb = lts, ''
        return (x, ea + textToSearch[xa:xb] + eb)

    def searchInMsgstr(self, text, context = 10):
        r = []
        for l, m, i, s, fuzzy in self.data:
            if i != "":
                index = -1
                while 1:
                    index, ctx = self.searchWordInText(text, s,
                                                       context, index + 1)
                    if index < 0: break
                    if ctx is not None: r.append((l, m, ctx))
        return r

    def getCleanMsgstr(self):
        s = self.getMsgstr()
        space = ('\\n', '\\t')
        empty = ('&', '_')
        import re
        s = re.sub('&[^;]*;', ' ', s)
        s = re.sub('<[^>]*>', ' ', s)
        s = re.sub('<[^>]*/>', ' ', s)
        s = re.sub('</[^>]*>', ' ', s)
        for t in space: s = s.replace(t, ' ')
        for t in empty: s = s.replace(t, '')
        return s

    def getTmp(self):
        msgstr = ''
        htmltags = ('ol', 'p', 'qt', 'br', 'li', 'ul',
                    'strong', 'b', 'i', 'em')
        for l, m, i, s, fuzzy in self.data:
            if i != "":
                s = s.replace('\\n', ' ').replace('&', '')
                for t in htmltags:
                    s = (s.replace('<%s>' % t, ' ').
                         replace('</%s>' % t, ' ').
                         replace('<%s/>' % t, ' '))
                msgstr = msgstr + s + ' '
        return msgstr

    def getErrors(self):
        return self.errors

    def hasErrors(self):
        return len(self.errors) > 0



class POTFile(POFile):

    def __init__(self, filename):
        POFile.__init__(self, filename)

    def check(self):
        self.errors = []
        for l, m, i, s, fuzzy in self.data:
            if len(i) == 0: continue
            req = capitalization.requiredCapitalization(i)
            cap = capitalization.capitalization(i)
            if req != capitalization.CAP_UNKNOWN and req != cap:
                self.errors.append((l, m, 'wrong capitalization - %s' % i))
