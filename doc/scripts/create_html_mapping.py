#!/usr/bin/env python
#
# Copyright (C) 2024 TortoiseGit team
# This file is distributed under the same license as TortoiseGit
#
# Author: Sven Strickroth <email@cs-ware.de>, 2024
#

'''
Developer tool to generate Conhext.h for the help mapping

Usage: create_html_mapping.py resource.h

Example:
	create_html_mapping.py ../src/TortoiseProc/resource.h > output/TortoiseGit_en/context.h

'''

import codecs
import re
import sys

STR_ID_MAP = {}

def find_Mappings(lines, idpart, offset, header = ''):
	global STR_ID_MAP
	err = False
	regexp_valid = re.compile(f'^#define\\s+[A-Za-z0-9_]+\\s+(?:0x[0-9]+|[1-9][0-9]*)$')
	regexp_entry = re.compile(f'^#define\\s+({idpart}[A-Za-z0-9_]+)\\s+(0x[0-9]+|[1-9][0-9]*)$')
	if header:
		print(header)
	for line in lines:
		line = line.rstrip()
		if line.startswith('// Next default values for new objects'):
			break
		if line.startswith('//') or not line:
			continue
		if not regexp_valid.search(line):
			print(f"Invalid line format found: <{line}>", file=sys.stderr)
			err = True
			continue
		m = regexp_entry.search(line)
		if not m:
			continue
		number = int(m.groups()[1], 0) + offset
		STR_ID_MAP[m.groups()[0]] = number
		print(f'#define H{m.groups()[0].ljust(38)} 0x{number:05X}')
	return err

def main():
	if len(sys.argv) < 2:
		print ("Usage: resource.h", file=sys.stderr)
		sys.exit(1)

	lines = codecs.open(sys.argv[1], 'r', 'utf-8').readlines()

	err = False

	print('// Generated Help Map file.')
	err |= find_Mappings(lines, 'IDM?_', 0x10000, '// Commands (ID_* and IDM_*)')
	err |= find_Mappings(lines, 'IDP_', 0x30000, '// Prompts (IDP_*)')
	err |= find_Mappings(lines, 'IDR_', 0x20000, '// Resources (IDR_*)')
	err |= find_Mappings(lines, 'IDD_', 0x20000, '// Dialogs (IDD_*)')

	sys.exit(1 if err else 0)

if __name__ == '__main__':
	main()
