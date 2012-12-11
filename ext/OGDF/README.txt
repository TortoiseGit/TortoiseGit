***********************************************************
*         OGDF - The Open Graph Drawing Framework         *
*                                                         *
*                         README                          *
***********************************************************

Welcome to OGDF!

OGDF is a portable C++ class library for graph drawing.
This archive contains the source-code of OGDF.


******************** LICENSE ********************

This software is distributed under the terms of the GNU
General Public License v2 or v3, with special exceptions
allowing to link against LP-solvers (see the LICENSE.txt
file for details!). By installing this software you agree
to these license terms.

If you have questions, please write to license@ogdf.net .


******************* COPYRIGHT *******************

All files in the OGDF distribution are copyrighted:

Copyright (C) 2005-2012


****************** INSTALLATION *****************

Unpack the OGDF archive in the directory, where you want to
install OGDF.

Build OGDF (gcc Compiler [Linux, Mac OS]):

  1. Edit makeMakefile.config for your configuration 
     (if necessary): check the [GENERAL] section. If 
     you do not use Coin, the default parameters should
     be suitable.

  2. Execute makeMakefile.sh to generate a suitable Makefile.

  3. Call make to build the OGDF library (you may also call
     make debug_all to generate a debuggable version).


Build OGDF (Visual Studio [Windows]):

  1. Create Visual Studio project file:
  
     Visual Studio 2008: Execute the python script makeVCProj.py
       to generate a Visual Studio 2008 project file ogdf.vcproj.
	   
     Visual Studio 2010: Execute the python script makeVCXProj.py
       to generate a Visual Studio 2010 project file ogdf.vcxproj.

  2. Open the created project file (.vcproj or .vcxproj) with
     Visual Studio and call build.

OGDF also contains some optional features which require COIN
Osi as LP solver. It is also possible to generate project
files for Visual Studio 2003 & 2005.

Please refer to the OGDF Wiki for more detailed information:

gcc:        http://www.ogdf.net/ogdf.php/tech:installgcc
Visual C++: http://www.ogdf.net/ogdf.php/tech:installvcc


******************** CHANGES ********************

For changes refer to the version history at

http://www.ogdf.net/doku.php?id=tech:versions


******************** CONTACT ********************

Email: info@ogdf.net

Web:   http://www.ogdf.net
Forum: http://www.ogdf.net/forum


Enjoy!

  The OGDF Team.
