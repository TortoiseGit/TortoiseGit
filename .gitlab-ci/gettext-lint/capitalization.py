# -*- coding: utf-8 -*-

# Capitalization checks
#
# Pedro Morais <morais@kde.org>
# José Nuno Pires <jncp@netcabo.pt>
# (c) Copyright 2003, 2004
# Distributable under the terms of the GPL - see COPYING

CAP_UNKNOWN = 1
CAP_TITLE = 2
CAP_SENTENCE = 3

def upperCaseWord(k):
    for i in k:
        if i[0].upper() != i[0]: return 0
    return i

def capitalization(s, ignoreUpper = (), ignoreLower = ()):
    if s.startswith("_: ") and s.find('\\n') > 0: s = s[s.find('\\n') + 2:]
    x = s.strip().split(' ')
    if len(x) < 1 or len(x[0]) < 1: return CAP_UNKNOWN
    f = x[0][0].upper() == x[0][0]
    uppercase = 0
    lowercase = 0

    for i in x[1:]:
        i = i.lstrip('_(&').rstrip('):.')
        if len(i) < 1: continue
        u = i[0].upper() == i[0]
        if u:
            if not upperCaseWord(i) and not i in ignoreUpper:
                uppercase = uppercase + 1
        else:
            if len(i) > 3 or not i in ignoreLower:
                lowercase = lowercase + 1

    if f == 1 and uppercase <= lowercase:
        return CAP_SENTENCE
    else:
        return CAP_TITLE

def requiredCapitalization(s):
    x = s.strip()
    l = len(x)
    if l > 0 and x[-1] in (':',):
        return CAP_SENTENCE
    return CAP_UNKNOWN
