<?php
//
// Drupal download page for the TortoiseSVN manuals
// loaded into:
// "http://tortoisesvn.net/support" 
//
// Copyright (C) 2004-2008 the TortoiseSVN team
// This file is distributed under the same license as TortoiseSVN
//
// $Author: luebbe $
// $Date: 2008-06-27 19:27:15 +0800 (Fri, 27 Jun 2008) $
// $Rev: 13360 $
//
// Author: Lübbe Onken 2004-2008
//         Stefan Küng 2004-2008
//

include("/var/www/vhosts/default/htdocs/modules/tortoisesvn/trans_countries.inc");
include("/var/www/vhosts/default/htdocs/modules/tortoisesvn/tortoisevars.inc");

$tsvn_var['devurl']='http://www.tortoisesvn.net/docs/nightly/';
$tsvn_var['relurl']='http://www.tortoisesvn.net/docs/release/';

if (!function_exists('print_manuals')) {
function print_manuals($i, $postat, $v, $b_release)
{
  $infobits=$postat[1];
  $flagimg=$v['flagpath']."$postat[2].png";

  if ( ($infobits & "110") <> "0") {

    if ($postat[2] == "gb") {
      $m_cc = "en";
      $m_cn = "English";
    } else {
      $m_cc = $postat[2];
      $m_cn = $postat[3];
    }

    $ts_pdf="TortoiseSVN-".$v['release'].'-'.$m_cc.".pdf";
    $tm_pdf="TortoiseMerge-".$v['release'].'-'.$m_cc.".pdf";
    $ts_htm="TortoiseSVN_".$m_cc."/index.html";
    $tm_htm="TortoiseMerge_".$m_cc."/index.html";

    if ($b_release==TRUE) {
      $pdfTSVN="<a href=\"".$v['url1'].$ts_pdf.$v['url2']."\">PDF</a>";
      $htmTSVN="<a href=\"".$v['relurl'].$ts_htm."\">HTML</a>";
      $pdfTMerge="<a href=\"".$v['url1'].$tm_pdf.$v['url2']."\">PDF</a>";
      $htmTMerge="<a href=\"".$v['relurl'].$tm_htm."\">HTML</a>";
    } else {
      $htmTSVN="<a href=\"".$v['devurl'].$ts_htm."\">HTML</a>";
      $htmTMerge="<a href=\"".$v['devurl'].$tm_htm."\">HTML</a>";
    }

    echo "<tr>";
    echo "<td><img src=\"$flagimg\" height=\"12\" width=\"18\" />&nbsp;$m_cn</td>";
    if ( ($infobits & "010") <> "0") {
      echo "<td>$pdfTSVN</td>";
      echo "<td>$htmTSVN</td>";
    }
    else {
      echo "<td>&nbsp;</td>";
      echo "<td>&nbsp;</td>";
    }
    if ( ($infobits & "100") <> "0") {
      echo "<td>$pdfTMerge</td>";
      echo "<td>$htmTMerge</td>";
    }
    else {
      echo "<td>&nbsp;</td>";
      echo "<td>&nbsp;</td>";
    }
    echo "</tr>";
  }

}
}
?>

There are several places where you can get support for TortoiseSVN.

<h2>Online documentation</h2>
<p>
<!-- SiteSearch Google --><form method="get" action="http://www.google.com/custom" target="_top"><table border="0" bgcolor="#ffffff"><tr><td nowrap="nowrap" valign="top" align="left" height="32"><br/><input type="hidden" name="domains" value="tortoisesvn.net"></input><input type="text" name="q" size="25" maxlength="255" value=""></input></td></tr><tr><td nowrap="nowrap"><table><tr><td><input type="radio" name="sitesearch" value=""></input><font size="-1" color="#000000">Web</font></td><td><input type="radio" name="sitesearch" value="tortoisesvn.net" checked="checked"></input><font size="-1" color="#000000">tortoisesvn.net</font></td></tr></table><input type="submit" name="sa" value="Google Search"></input><input type="hidden" name="client" value="pub-0430507460695576"></input><input type="hidden" name="forid" value="1"></input><input type="hidden" name="ie" value="ISO-8859-1"></input>
<input type="hidden" name="oe" value="ISO-8859-1"></input><input type="hidden" name="cof" value="GALT:#008000;GL:1;DIV:#336699;VLC:663399;AH:center;BGC:FFFFFF;LBGC:336699;ALC:0000FF;LC:0000FF;T:000000;GFNT:0000FF;GIMP:0000FF;FORID:1"></input>
<input type="hidden" name="hl" value="en"></input></td></tr></table></form><!-- SiteSearch Google -->
</p>


<?php

// Convert Data into a list of columns
foreach ($countries as $key => $row) {
   $potfile[$key] = $row[0];
   $country[$key] = $row[3];
}

// Add $countries as the last parameter, to sort by the common key
array_multisort($potfile, $country, $countries);

?>

<h1>Manuals (release version)</h1>
<p>If you have TortoiseSVN installed, you can simply press the F1 key in any dialog to start up the help. That help is the same as the documentation you find here.
<div class="table">
<table>
<tr>
<th class="lang">Language</th>
<th class="lang" colspan="2">TortoiseSVN</th>
<th class="lang" colspan="2">TortoiseMerge</th>
</tr>
<?php
$i=0;
foreach ($countries as $key => $postat)
{
  if ($postat[0] >= "0" ) {
    $i++;
    print_manuals($i, $postat, $tsvn_var, TRUE);
  }
}
?>
</table>
</div>
</p>

<h1>Manuals (developer version)</h1>
<p>These manuals are only for the trunk build, not released versions. Please note that these docs aren't updated nightly but very irregularly.
<div class="table">
<table>
<tr>
<th class="lang">Language</th>
<th class="lang" colspan="2">TortoiseSVN</th>
<th class="lang" colspan="2">TortoiseMerge</th>
</tr>
<?php
$i=0;
foreach ($countries as $key => $postat)
{
  if ($postat[0] >= "0" ) {
    $i++;
    print_manuals($i, $postat, $tsvn_var, FALSE);
  }
}
?>
</table>
</div>
</p>

<h1>Older Manuals</h1>
<p>Older releases are available from the <a href="http://sourceforge.net/project/showfiles.php?group_id=<?php print $tsvn_var['sf_project']; ?>">Sourceforge files</a> section.</p>

<h1>Project Status</h1>
<p>Have a look at our <a href="/status">project status</a> page to see what we are working on at the moment, and to check the release history.</p>

<h1>Subversion book</h1>
<p>Read the official Subversion book <a href="http://www.amazon.com/gp/product/0596004486?ie=UTF8&tag=to0d-20&linkCode=as2&camp=1789&creative=9325&creativeASIN=0596004486">Version Control with Subversion</a><img src="http://www.assoc-amazon.com/e/ir?t=to0d-20&l=as2&o=1&a=0596004486" width="1" height="1" border="0" alt="" style="border:none !important; margin:0px !important;" /> to find out what it's all about. This book explains the general concepts of Subversion. It's no must, but it'll give you deep insight. There's also a <a href="http://svnbook.red-bean.com/">free online version</a> available.</p>

<h1>FAQ</h1>
<p>A list of common problems and their solutions can be found in the <a href="/faq">FAQ</a>. Please note that the FAQ contains answers, but is not the place to ask questions. For that you need to go to the users mailing list. When we have a good answer to a good question we post it in the FAQ.</p>
<!--break-->

<h1>Mailing list</h1>
<p>If your question is not answered in any of these places, you can subscribe to one of our two mailing lists:
<ul>
<li>The users list, where we deal with the howto (configuration, daily work, ...). If there is something you don't understand, feel free to ask a question here and you will usually get a rapid response.</li>
<li>The developer list, where we discuss the internals of TortoiseSVN (bugs, features, enhancements, ...). If you want to contribute to the project, this is where the development discussions take place.</li>
</ul>
Before you post/subscribe to one of these lists, please read our <a href="http://tortoisesvn.tigris.org/list_etiquette.html">list etiquette</a>. Also please search the archives before posting questions. Many questions have been asked before.</p>
<p>
You can subscribe to our mailing lists <a href="http://tortoisesvn.tigris.org/servlets/ProjectMailingListList">here</a>.
</p>
<p>If you want to search the lists, our mailing lists are archived on gmane.org and haxx.se. You can either access the archives via http: or nntp: protocol. Haxx.se is recommended for searches.</p>

<h3>Users list archive</h3>
<p><ul>
<li><a href="http://groups.google.com/group/tortoisesvn">via web browser (Google Groups)</a></li>
<li><a href="http://svn.haxx.se/tsvnusers/">via web browser (haxx)</a></li>
<li><a href="nntp://news.gmane.org/gmane.comp.version-control.subversion.tortoisesvn.user">via news reader</a></li></ul></p>
<h3>Developer list archive</h3>
<p><ul>
<li><a href="http://groups.google.com/group/tortoisesvn-dev">via web browser (Google Groups)</a></li>
<li><a href="http://svn.haxx.se/tsvn/">via web browser (haxx)</a></li>
<li><a href="nntp://news.gmane.org/gmane.comp.version-control.subversion.tortoisesvn.devel">via news reader</a></li></ul></p>

<h1>RSS Feed</h1>
<p>The latest updates on our project web page are always visible in a <a href="node/feed">RSS feed</a>.</p>
