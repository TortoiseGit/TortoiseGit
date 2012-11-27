#!/usr/bin/env python
# Make Makefile
#
# Jul 28, 2005
# Markus Chimani, markus.chimani@cs.uni-dortmund.de
#########################################################

import os, sys, fnmatch, ConfigParser, posixpath

class versionclass: 
	def call(self):
		return '$(' + self.var + ')'
	def library(self):
		return self.call() + '/' + libName
	def sharedlibrary(self):
		return self.call() + '/' + sharedlibName
	def objects(self):
		return '$(' +self.var + '_OBJS)'
	def path(self):
		return '_' + self.var

def bailout(msg):
	print msg
	print 'Please use the original makeMakefile.config as a template'
	sys.exit()
	
def loadConfig(sect, key, noError = False ):
	if config.has_option(sect, key):
		v = config.get(sect, key)
		print '   [', sect, ']', key, '=', v
		return v
	else:
		if noError:
			return None		
		else:
			bailout('Option "' + key + '" in section "' + sect + '" is missing')

#########################################################
# LOAD CONFIGURATION

config = ConfigParser.ConfigParser()
print 'Loading makeMakefile.config...'

try:
	config.readfp( open('makeMakefile.config') )
except IOError:
	bailout('makeMakefile.config not found')

if not config.has_section('GENERAL'):
	bailout('Section "GENERAL" is missing')
if not config.has_section('VERSIONS'):
	bailout('Section "VERSIONS" is missing')
if not config.has_section('COIN'):
	bailout('Section "COIN" is missing')
if not config.has_section('ABACUS'):
	bailout('Section "ABACUS" is missing')

sharedLib = loadConfig('GENERAL', 'sharedLib').startswith('t')
libName = loadConfig('GENERAL', 'libName')
sharedlibName = loadConfig('GENERAL', 'sharedlibName')
compilerCommand = loadConfig('GENERAL', 'compilerCommand')
compilerParams = loadConfig('GENERAL', 'compilerParams')
libCommand = loadConfig('GENERAL', 'libCommand')
sharedlibCommand = loadConfig('GENERAL', 'sharedlibCommand')
rmCommand = loadConfig('GENERAL', 'rmCommand')
mkdirCommand = loadConfig('GENERAL', 'mkdirCommand')
includeLegacyCode = loadConfig('GENERAL', 'includeLegacyCode').startswith('t')
useOwnLpSolver = loadConfig('GENERAL', 'useOwnLpSolver').startswith('t')

ranlibCommand = loadConfig('GENERAL', 'ranlibCommand', True)
if ranlibCommand == None:
	ranlibCommand = ''

gccMessageLength = loadConfig('GENERAL', 'gccMessageLength', True)
if gccMessageLength == None:
	gccMessageLength = ''
else:
	gccMessageLength = '-fmessage-length=' + gccMessageLength

compiler = ' '.join( [ compilerCommand, gccMessageLength, compilerParams ] )

libs = ''

if sharedLib:
	compiler = ' '.join( [compiler, '-DOGDF_DLL -DOGDF_INSTALL' ] )
	if sys.platform == 'win32' or sys.platform == 'cygwin':
		libs = ' '.join( [libs, '-lpsapi'] )
	else:
		compiler = ' '.join( [compiler, '-fPIC'] )
	
if useOwnLpSolver:
	compiler = ' '.join( [compiler, '-DOGDF_OWN_LPSOLVER' ] )

useCoin = loadConfig('COIN', 'useCoin').startswith('t')
if useCoin:
	coinIncl = loadConfig('COIN', 'coinIncl')
	# coinLib = loadConfig('COIN', 'coinLib')
	solver_name = loadConfig('COIN', 'solver_name')
	solver_incl = loadConfig('COIN', 'solver_incl')
	# solver_lib = loadConfig('COIN', 'solver_lib')
	si2 = ' '
	if solver_incl.strip() != '':
	  si2 = '-I'+solver_incl
	compiler = ' '.join( [ compiler, '-I'+coinIncl, si2, '-D'+solver_name, '-DUSE_COIN', ' ' ] )
	
useAbacus = loadConfig('ABACUS', 'useAbacus').startswith('t')
if useAbacus:
	abacusDef = loadConfig('ABACUS', 'abacusDef')
	abacusIncl = loadConfig('ABACUS', 'abacusIncl')
	# abacusLib = loadConfig('ABACUS', 'abacusLib')
	compiler = ' '.join( [ compiler, abacusDef, '-I'+abacusIncl, '-DUSE_ABACUS', ' ' ] )
	
versions = []
V = config.items('VERSIONS')
if len(V) == 0:
	bailout('Versions missing')
else:
	for ve in V:
		v = versionclass()
		v.var, v.params = ve
		print '   [ VERSIONS ] Name:', v.var, ', Cmd:',v.params
		versions.append(v)

print 'Resulting compiler call:', compiler

print 'Finished loading makeMakefile.config'

#########################################################
# ANALYZE & GENERATE

print 'Analyzing sources & generating Makefile...'

makefile = open('Makefile','w')

# add header
header = open('Makefile.header')
headercontent = header.read()
header.close()
makefile.write(headercontent)

# define release & debug

for v in versions:
	makefile.write(v.var + ' = ' + v.path() + '\n')
makefile.write('\n');

# just the def. nothing happens yet
def Walk( curdir ):

	objs = []
	names = os.listdir( curdir)
	names.sort()
	
	for name in names:
		if name.startswith('.') or name.startswith('_') or (name=='legacy' and not includeLegacyCode):
			continue

		fullname = posixpath.normpath(posixpath.join(curdir, name))
		
		if os.path.isdir(fullname) and not os.path.islink(fullname):
			objs = objs + Walk( fullname )
		else:
			for pat in [ '*.c', '*.cpp' ]:
				if fnmatch.fnmatch(name, pat):
					objfullname = fullname[:-len(pat)+2] + 'o'
					objs.append(objfullname)
					
					callForDeps = callForDepsBase + fullname + ' > targetAndDepend'
					os.system( callForDeps )					
					t = open('targetAndDepend')
					targetAndDepend = t.read()
					t.close()
					
					for v in versions:
						# print target&depend: add full path spec, incl. version & ignore extra line
						path = v.call() + '/' +fullname[:-len(name)]
						makefile.write(path + targetAndDepend[:-1] + '\n')

						# ensure folder					
						makefile.write('\t' + mkdirCommand + ' ' + v.call() + '/' + fullname[:-len(name)-1] + '\n')
						# what to do: call the compiler
						makefile.write('\t' + compiler + ' ' + v.params + ' -o ' + v.call() + '/' + objfullname + ' -c ' + fullname + '\n\n')
					
					# pattern found: don't try other suffix
					break			
	return objs

callForDepsBase = compiler + ' -MM ';
if useCoin:
	callForDepsBase += '-DUSE_COIN -D' + solver_name + ' '
if useAbacus:
	callForDepsBase += '-DUSE_ABACUS -DABACUS_COMPILER_GCC '
 	
# Call recursive function
objs = Walk( '.' )
# Clean up
os.system(rmCommand + ' targetAndDepend')

# List all Objs for use in lib-generation and clear
for v in versions:
	makefile.write(v.objects()[2:-1] + ' = \\\n')
	for o in objs:
		makefile.write(v.call() + '/' + o + ' \\\n')
	makefile.write('\n')

# generate alls and cleans etc...

for v in versions:
	
	if sharedLib:
		makefile.write(v.var + ': ' + v.sharedlibrary() + '\n\n')
		makefile.write(v.sharedlibrary() + ': ' + v.objects() + '\n')
		makefile.write('\t' + sharedlibCommand  + ' -shared -o ' + v.sharedlibrary() + ' ' + v.objects() + ' ' + libs + ' $(LIBS)\n')

	else:
		makefile.write(v.var + ': ' + v.library() + '\n\n')
		makefile.write(v.library() + ': ' + v.objects() + '\n')
		makefile.write('\t' + libCommand + ' -r ' + v.library() + ' '  + v.objects() + ' $(LIBS)\n')
		if ranlibCommand != '':
			makefile.write('\t' + ranlibCommand + ' ' + v.library() + '\n')
	
	makefile.write('\n')
	makefile.write('clean' + v.var + ':\n')
#	makefile.write('\t' + rmCommand + ' ' + v.objects() + ' ' + v.library() + '\n\n')
	makefile.write('\t' + rmCommand + ' ' + v.path() + '\n\n')
	
makefile.close()

print 'Makefile generated'


