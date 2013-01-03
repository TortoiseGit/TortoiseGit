HowTo build the docs
====================

Since you are already reading this, I assume that you have succeeded in checking 
out the TortoiseGit.

Tools needed:
=============

There are some tools for processing the XML input that you need to build the docs.
Scripts and dtd are included, but the executables (formatting processor, microsoft
help compiler, translation tools) have to be installed separately.
You will also need to have a Java Runtime Environment version 1.3.x or above.

tools\fop\		- the fop processor
tools\xsl\		- the docbook xsl files from sourceforge
tools\			- xsl processor, hhc.exe, ...

you can download all the required tools as a zip package from our website:
http://code.google.com/p/tortoisesvn/downloads/list
tools-*.7z. Use 7-zip extract to \TortotiseGit\Tools


Please note that having spaces in your directory path will (for the time being)
cause the documentation build process to fail.

To build only the english docs, that's all you need.

For Chm docs you need:
- Microsofts makehm.exe, Part of visual studio, sources available on msdn
- Microsofts html workshop, Binaries available on msdn


Structure:
==========
The most important directories for you are:
source\en - contains the english XML text source.
images\en - contains the base (english) images for the docs.
xsl\	  - contains the stylesheets for the doc creation
dtd\      - contains the tools and the dtd to validate and build the docs.
	    You might want to place your tools directory somewhere else on your 
            harddisk, if you want to use it to build other docs too. This will 
            however require tweaking the build scripts.
            I'd recommend to leave dtd in place, so the source stays 
            compatible between TSVN doc developers.
            
Building the docs:
==================

NAnt Build:
-----------

VS->Tools->Visual Studio 2012 command line
cd doc
..\tools\nant-0.92\bin\nant.exe

A NAnt build script has been provided to build the docs. When doc.build is run for
the first time, the template doc.build.include.template is copied to doc.build.include.

For local customisations, copy the doc.build.user.tmpl file to doc.build.user and
modify that copy to point to the location of the tools on your system.

All other parameters are defined in doc.build.include. You can override all settings
in doc.build.user or via the NAnt command line.
