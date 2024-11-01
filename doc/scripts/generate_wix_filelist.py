#!/usr/bin/env python
#
# Copyright (C) 2024 the TortoiseGit team
# This file is distributed under the same license as TortoiseGit
#
# Author: Sven Strickroth <email@cs-ware.de>, 2024
#

'''
Developer tool to generate the WiX file list for the HTML help files

Works similar to the WiX harvesting tool Heat.

Usage:
	generate_wix_filelist.py

Example:
	generate_wix_filelist.py > ../src/TortoiseGitSetup/HTMLHelpfiles.wxi
'''

import sys
import os
from xml.etree.ElementTree import Element, SubElement, tostring
from xml.dom import minidom
import hashlib
import uuid

def generate_deterministic_guid(seed_string):
	return str(uuid.UUID(hashlib.md5(seed_string.encode('utf-8')).hexdigest()))

def generate_wix_directory(root_dir, directory_path, wix_dir, current_path=None, is_first_level=True):
	current_path = current_path or {}
	for root, dirs, files in os.walk(directory_path):
		rel_dir = os.path.relpath(root, root_dir).replace('\\', '_').replace('/', '_')
		dir_id = f"D__htmlhelp_{rel_dir}"

		# Skip creating a Directory tag for the first directory
		if is_first_level:
			dir_element = wix_dir
			is_first_level = False
		else:
			# Check if the directory has been processed before
			if dir_id not in current_path:
				dir_name = os.path.basename(root)
				dir_element = SubElement(wix_dir, "Directory", Id=dir_id, Name=dir_name)
				current_path[dir_id] = dir_element
			else:
				dir_element = current_path[dir_id]

		# Add files to the current directory
		for file in files:
			if file == "alias.h":
				continue
			if file == "context.h" or file.endswith('.chm'):
				raise Exception("Documentation needs to be generated using the docformat 'html'")
			file_path = os.path.join(root, file).replace('/', '\\')
			file_id = f"F__htmlhelp_{rel_dir}_{os.path.splitext(file)[0]}".replace('-', '_').replace(' ', '_')
			component_guid = generate_deterministic_guid(file_id)
			component = SubElement(dir_element, "Component", Id=f"C__{file_id[3:]}", Guid=component_guid, Win64="$(var.Win64YesNo)")
			SubElement(component, "File", Id=file_id, Name=file, Source=f"..\\..\\doc\\{file_path}")

		# Add subdirectories as child elements
		for subdir in dirs:
			subdir_path = os.path.join(root, subdir)
			sub_rel_dir = os.path.relpath(subdir_path, root_dir).replace('\\', '_').replace('/', '_')
			dir_name = subdir
			if dir_name == "TortoiseMerge_en":
				dir_name = "TortoiseGitMerge_en"
			sub_dir_id = f"D__htmlhelp_{sub_rel_dir}"
			current_path[sub_dir_id] = SubElement(dir_element, "Directory", Id=sub_dir_id, Name=dir_name)

def main():
	wix_root = Element("Include")
	base_dir = SubElement(wix_root, "DirectoryRef", Id="D__Bin")

	directory_path = 'output'

	generate_wix_directory(directory_path, directory_path, base_dir)

	compontentgroup = SubElement(wix_root, "ComponentGroup", Id="C__help_en")
	for component in base_dir.findall(".//Component"):
		SubElement(compontentgroup, "ComponentRef", Id=component.attrib["Id"])

	sys.stdout.write(minidom.parseString(tostring(wix_root)).toprettyxml(indent="\t"))

if __name__ == '__main__':
	main()
