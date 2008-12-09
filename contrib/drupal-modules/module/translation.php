<?php
//
// Drupal translation status start page
// loaded into "http://tortoisesvn.net/translation"
//
// Copyright (C) 2004-2008 the TortoiseSVN team
// This file is distributed under the same license as TortoiseSVN
//
// $Author: luebbe $
// $Date: 2008-06-23 20:38:59 +0800 (Mon, 23 Jun 2008) $
// $Rev: 13328 $
//
// Author: Lübbe Onken 2004-2008
//

include("/var/www/vhosts/default/htdocs/modules/tortoisesvn/tortoisevars.inc");

?>

Would you like to translate TortoiseSVN into your native language?

To check the currently available languages, take a look at our
<a href="/translation_status">Translation status page</a>. Maybe your language
already exists and is ready for use or needs just a little improvement. Read
these <a href="/translation_instructions">instructions</a> first before starting
to work on a translation.

<ul>
<li>If you want to translate the current development version, download an
<a href="<?php echo $tsvn_var['trunk']; ?>Languages/Tortoise.pot">empty catalog (trunk)</a>.
Translate what you can and send us your translation.
</li>
<li>If you want to translate the last official release, download an
<a href="<?php echo $tsvn_var['branch']; ?>Languages/Tortoise.pot">
empty catalog (<?php echo $tsvn_var['release']; ?> )</a> and start to translate from there.
</li>
</ul>

When you are finished, send us your translation (just the zipped .po file) to
<a href="mailto:dev@tortoisesvn.tigris.org">dev@tortoisesvn.tigris.org</a>.
We will run some sanity checks on it and commit it into our repository.
From that moment on, an installer for your language will be available in the
nightly developer builds for testing. It'll become part of the nexet official
release. We will also put your name on the <a href="/translator_credits">list of contributors</a>.

Now go ahead and <a href="/downloads">download</a> your favourite language pack.
<!--break-->
