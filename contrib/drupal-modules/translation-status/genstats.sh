#!/bin/sh
#
# Create the doc and gui statistics
#
#
# Copyright (C) 2004-2008 the TortoiseSVN team
# This file is distributed under the same license as TortoiseSVN
#
# $Author: luebbe $
# $Date: 2008-06-23 20:38:59 +0800 (Mon, 23 Jun 2008) $
# $Rev: 13328 $
#
# Author: Lübbe Onken 2004-2008
#

# First 'svn cleanup' the externals manually, because cleanup doesn't walk into externals
# Afterwards 'svn up' everything
(cd trunk/gui; svn cleanup > /dev/null 2>&1)
(cd trunk/doc; svn cleanup > /dev/null 2>&1)
(cd branch/doc; svn cleanup > /dev/null 2>&1)
(cd branch/gui; svn cleanup > /dev/null 2>&1)
(svn cleanup > /dev/null 2>&1; svn up > /dev/null 2>&1)

# Create doc statistics for trunk
./get-doc-stats.sh trunk >trans_data_trunk.inc

# Create gui statistics
./get-gui-stats.sh trunk >>trans_data_trunk.inc

# Create doc statistics for branch
./get-doc-stats.sh branch >trans_data_branch.inc

# Create gui statistics
./get-gui-stats.sh branch >>trans_data_branch.inc

# Copy results into web space
cp trans_*.inc htdocs/modules/tortoisesvn/
cp tortoisevars.inc htdocs/modules/tortoisesvn/
