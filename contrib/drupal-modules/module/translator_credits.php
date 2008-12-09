<!--break-->
<?php
//
// Drupal translator credits page
// loaded into "http://tortoisesvn.net/translator_credits"
//
// Copyright (C) 2004-2008 the TortoiseSVN team
// This file is distributed under the same license as TortoiseSVN
//
// $Author: steveking $
// $Date: 2008-07-25 02:17:50 +0800 (Fri, 25 Jul 2008) $
// $Rev: 13536 $
//
// Author: Lübbe Onken 2004-2008
//

include("/var/www/vhosts/default/htdocs/modules/tortoisesvn/trans_data_trunk.inc");
include("/var/www/vhosts/default/htdocs/modules/tortoisesvn/trans_countries.inc");
include("/var/www/vhosts/default/htdocs/modules/tortoisesvn/tortoisevars.inc");

function t_print_header($tsvn_var)
{
?>

<div class="content">
<p>
On this page we want to give credit to everyone who has contributed to the many translations we now have. I hope I haven't forgotten a translator... Thanks everybody!
</p>

<?php
}

function t_print_footer($tsvn_var)
{
?>

</div>

<?php
}

function t_print_table_header($name, $summary, $tsvn_var)
{
?>
<h2><?php echo $summary ?></h2>
<div class="table">
<table class="translations" summary="<?php echo $summary ?>">
<tr>
<th class="lang">Nr.</th>
<th class="lang">Language</th>
<th class="lang">Translator(s)</th>
</tr>
<?php
}

function t_print_table_footer()
{
?>
</table>
</div>
<div style="clear:both">&nbsp;<br/></div>
<?php
}

function t_print_content_stat($i, $postat, $poinfo, $tsvn_var)
{
  $release=$tsvn_var['release'];
  $build=$tsvn_var['build'];
  $dlfile=$tsvn_var['url1']."LanguagePack_".$release.".".$build."-win32-".$poinfo[2].".msi".$tsvn_var['url2'];

  if ($poinfo[0] != '') {
    $flagimg=$tsvn_var['flagpath']."$poinfo[2].png";

    echo "<td>$i</td>";
    echo "<td class=\"lang\"><img src=\"$flagimg\" height=\"12\" width=\"18\" />&nbsp;<a href=\"$dlfile\">$poinfo[3]</a></td>";
    echo "<td class=\"lang\">$poinfo[4]</td>";
  }
}

function t_print_all_stats($data, $countries, $tsvn_var)
{
  $i=0;
  foreach ($data as $key => $postat)
    if ($postat[0] == 0) {
      $i++;
      echo "<tr>";
      t_print_content_stat($i, $postat, $countries[$key], $tsvn_var);
      echo "</tr>";
    }
}

//------------------------------------
//
// The program starts here
//

t_print_header($tsvn_var);

// Print Alphabetical statistics
t_print_table_header('alpha', 'Translator credits', $tsvn_var);
t_print_all_stats($TortoiseGUI, $countries, $tsvn_var);
t_print_table_footer();

t_print_footer($tsvn_var);
?>
