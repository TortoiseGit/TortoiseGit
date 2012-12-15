#!/usr/bin/env python
# Make VCXProj
#
# May 2010
# Markus Chimani, markus.chimani@cs.tu-dortmund.de
# Carsten Gutwenger, carsten.gutwenger@cs.tu-dortmund.de
#########################################################


import os, sys, fnmatch, ConfigParser

class stuff:
	def __init__(self, ttag, tpath, tpats, tcommand, tfilter):
		self.tag, self.path, self.pats, self.command, self.filter = ttag, tpath, tpats, tcommand, tfilter

def bailout(msg):
	print msg
	print 'Please use the original makeVCXProj.config as a template'
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

makecfg = 'makeVCXProj.config'
for x in sys.argv[1:]:
    if x[:7] == "config=":
        makecfg = x[7:]

print 'Loading ' + makecfg + '...'

try:
	config.readfp( open(makecfg))
except IOError:
	bailout(makecfg + ' not found')

if not config.has_section('GENERAL'):
	bailout('Section "GENERAL" is missing')
if not config.has_section('COIN'):
	bailout('Section "COIN" is missing')

#########################################################
# CONFIGS

# Filenames
filename_vcxproj =  loadConfig('GENERAL', 'projectFile')
filename_template = loadConfig('GENERAL', 'templateFile')
filename_vcxfilters =  loadConfig('GENERAL', 'projectFiltersFile')
filename_template_filters = loadConfig('GENERAL', 'templateFiltersFile')
addIncludes = ''
addDefines = ''
addLibs = ''
addLibPathsDebugWin32 = ''
addLibPathsReleaseWin32 = ''
addLibPathsDebugX64 = ''
addLibPathsReleaseX64 = ''

useOwnLpSolver = loadConfig('GENERAL', 'useOwnLpSolver', 'false').startswith('t')
if useOwnLpSolver:
	addDefines += 'OGDF_OWN_LPSOLVER;'
	
useCoin = loadConfig('COIN', 'useCoin').startswith('t')
if useCoin:
	coinIncl = loadConfig('COIN', 'coinIncl')
	coinLib = loadConfig('COIN', 'coinLib')
	solver_name = loadConfig('COIN', 'solver_name')
	solver_incl = loadConfig('COIN', 'solver_incl')

	addDefines += 'USE_COIN;'+solver_name+';'
	addIncludes += coinIncl+';'
	if solver_incl.strip() != '':
		addIncludes += solver_incl+';'
	addLibs += 'libCoinUtils.lib libOsi.lib '
	addLibs += 'libClp.lib libOsiClp.lib '
	addLibPathsDebugWin32 += coinLib+'/win32/Debug;'
	addLibPathsReleaseWin32 += coinLib+'/win32/Release;'
	addLibPathsDebugX64 += coinLib+'/x64/Debug;'
	addLibPathsReleaseX64 += coinLib+'/x64/Release;'

addDefines = addDefines[:-1]
addIncludes = addIncludes[:-1]
addLibs = addLibs[:-1]
addLibPathsDebugWin32 = addLibPathsDebugWin32[:-1]
addLibPathsReleaseWin32 = addLibPathsReleaseWin32[:-1]
addLibPathsDebugX64 = addLibPathsDebugX64[:-1]
addLibPathsReleaseX64 = addLibPathsReleaseX64[:-1]
defineTag = '<<DEFINETAG>>'
includeTag = '<<INCLUDETAG>>'
libTag = '<<LIBTAG>>'
libPathsTagDebugWin32 = '<<LIBPATHSDEBUGWIN32TAG>>'
libPathsTagReleaseWin32 = '<<LIBPATHSRELEASEWIN32TAG>>'
libPathsTagDebugX64 = '<<LIBPATHSDEBUGX64TAG>>'
libPathsTagReleaseX64 = '<<LIBPATHSRELEASEX64TAG>>'
filtersTag = '<<FTAG>>'

# Params are: 
# - Tag in template-File
# - Directory to start search & subfilters from
# - File Patterns
cppStuff = stuff( '<<CPPTAG>>', 'src', [ '*.c', '*.cpp' ], 'ClCompile', 'Source Files' )
hStuff = stuff( '<<HTAG>>', 'ogdf', [ '*.h' ], 'ClInclude', 'Header Files' )
hLegacyStuff = stuff( '<<HLEGACYTAG>>', 'ogdf_legacy', [ '*.h' ], 'ClInclude', 'Header Files Legacy' )

includeStuff = stuff

stuff = [ cppStuff, hStuff, hLegacyStuff ]


#########################################################
#########################################################
## only code below...

# just the def. nothing happens yet
def Walk( curdir, pats, command ):
	names = os.listdir( curdir)
	names.sort()
	
	for name in names:
		if name.startswith('.') or name.startswith('_') or (name=='legacy' and not includeLegacyCode):
			continue

		outpath = curdir + '\\' + name
		fullname = os.path.normpath(outpath)
		
		if os.path.isdir(fullname) and not os.path.islink(fullname):
			Walk( outpath, pats, command)
		else:
			for pat in pats:
				if fnmatch.fnmatch(name, pat):
					vcxproj.write('    <' + command + ' Include="' + outpath + '" />\n')

def WalkFilterFiles( curdir, pats, command, filter ):
	names = os.listdir( curdir)
	names.sort()
	
	for name in names:
		if name.startswith('.') or name.startswith('_') or (name=='legacy' and not includeLegacyCode):
			continue

		outpath = curdir + '\\' + name
		fullname = os.path.normpath(outpath)
		
		if os.path.isdir(fullname) and not os.path.islink(fullname):
			WalkFilterFiles( outpath, pats, command, filter + '\\' + name )
		else:
			for pat in pats:
				if fnmatch.fnmatch(name, pat):
					vcxfilters.write('    <' + command + ' Include="' + outpath + '">\n')
					vcxfilters.write('      <Filter>' + filter + '</Filter>\n')
					vcxfilters.write('    </' + command + '>\n')

def WalkFilters( curdir, filter ):
	names = os.listdir( curdir)
	names.sort()
	
	for name in names:
		if name.startswith('.') or name.startswith('_') or (name=='legacy' and not includeLegacyCode):
			continue

		outpath = curdir + '\\' + name
		fullname = os.path.normpath(outpath)
		
		if os.path.isdir(fullname) and not os.path.islink(fullname):
			filtername = filter + '\\' + name
			vcxfilters.write('    <Filter Include="' + filtername + '">\n')
			vcxfilters.write('    </Filter>\n')
			WalkFilters( outpath, filtername )


##########################################
## Main below...

print 'Generating VCXProj...'

includeLegacyCode = 0;
if len(sys.argv)>1 and sys.argv[1]=='legacy':
	includeLegacyCode = 1
	print '(including legacy code)'

vcxproj = open(filename_vcxproj,'w')
template = open(filename_template)

check = 0
for line in template:
	if check < len(stuff) and line.find(stuff[check].tag) > -1:
		if (stuff[check].tag!='<<HLEGACYTAG>>' or includeLegacyCode):
			Walk(stuff[check].path, stuff[check].pats, stuff[check].command)
		check = check + 1
	elif line.find(defineTag) > -1:
		vcxproj.write(line.replace(defineTag,addDefines,1))
	elif line.find(includeTag) > -1:
		vcxproj.write(line.replace(includeTag,addIncludes,1))
	elif line.find(libTag) > -1:
		vcxproj.write(line.replace(libTag,addLibs,1))
	elif line.find(libPathsTagDebugWin32) > -1:
		vcxproj.write(line.replace(libPathsTagDebugWin32,addLibPathsDebugWin32,1))
	elif line.find(libPathsTagReleaseWin32) > -1:
		vcxproj.write(line.replace(libPathsTagReleaseWin32,addLibPathsReleaseWin32,1))
	elif line.find(libPathsTagDebugX64) > -1:
		vcxproj.write(line.replace(libPathsTagDebugX64,addLibPathsDebugX64,1))
	elif line.find(libPathsTagReleaseX64) > -1:
		vcxproj.write(line.replace(libPathsTagReleaseX64,addLibPathsReleaseX64,1))
	else:
		vcxproj.write(line)

template.close()
vcxproj.close()

# Creation of filters file...

vcxfilters = open(filename_vcxfilters,'w')
template_filters = open(filename_template_filters)

check = 0
for line in template_filters:
	if check < len(stuff) and line.find(stuff[check].tag) > -1:
		if (stuff[check].tag!='<<HLEGACYTAG>>' or includeLegacyCode):
			WalkFilterFiles(stuff[check].path, stuff[check].pats, stuff[check].command, stuff[check].filter)
		check = check + 1
	elif line.find(filtersTag) > -1:
		for s in stuff:
			if (s.tag!='<<HLEGACYTAG>>' or includeLegacyCode):
				WalkFilters(s.path, s.filter)
	else:
		vcxfilters.write(line)


template_filters.close()
vcxfilters.close()

		 
print 'VCXProj generated'
