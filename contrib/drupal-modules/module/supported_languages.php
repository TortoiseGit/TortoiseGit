<!--break-->
<?php
// index.php
//
// Main page.  Lists all the translations

include("/var/www/vhosts/default/htdocs/modules/tortoisesvn/trans_data_trunk.inc");
include("/var/www/vhosts/default/htdocs/modules/tortoisesvn/trans_countries.inc");

$vars['release']=variable_get('tsvn_version', '');
$vars['build']=variable_get('tsvn_build', '');
$vars['downloadurl1']=variable_get('tsvn_sf_prefix', '');
$vars['downloadurl2']=variable_get('tsvn_sf_append', '');
$vars['reposurl']=variable_get('tsvn_repos_trunk', '').'Languages/';
$vars['flagpath']="/flags/world.small/";

$basename="Tortoise";
$template=$basename.".pot";

function s_print_content_stat($i, $postat, $poinfo, $vars)
{

  $release=$vars['release'];
  $build=$vars['build'];
  $dlfile=$vars['downloadurl1']."LanguagePack-".$release.".".$build."-win32-".$poinfo[2].".exe".$vars['downloadurl2'];

  if ($poinfo[0] == '') {
    $flagimg=$vars['flagpath']."gb.png";
  } else {
    $flagimg=$vars['flagpath']."$poinfo[2].png";
  }

  echo "<a href=\"$dlfile\"><img src=\"$flagimg\" height=\"12\" width=\"18\" alt=\"$poinfo[1]\" title=\"$poinfo[3]\" /></a>&nbsp;";
}

function s_print_single_stat($i, $postat, $poinfo, $vars)
{
  if (($postat[0] > 0) || ($postat[1] == $postat[3])){
    // error
    s_print_content_stat($i, $postat, $poinfo, $vars);
  }
  else if ($postat[1] == 0) {
    // no translations
//    s_print_content_stat($i, $postat, $poinfo, $vars);
  }
  else {
    // everything ok
    s_print_content_stat($i, $postat, $poinfo, $vars);
  }
}

function s_print_all_stats($data, $countries, $vars)
{
  $i=0;
  foreach ($data as $key => $postat)
  {
      $i++;
      s_print_single_stat($i, $postat, $countries[$key], $vars);
  }
}

//------------------------------------
//
// The program starts here
//
?>

<div class="content">
<h2>TortoiseSVN <?php echo $vars['release'] ?> Translations</h2>

<p>
The following <?php echo count($TortoiseGUI); ?> languages are supported by TortoiseSVN
</p>
<p>
<?php s_print_all_stats($TortoiseGUI, $countries, $vars) ?>
</p>
<p>
Find out more on our <a href="translation_status">Translation status page</a>.
See how many <a href="translator_credits">volunteers</a> have contributed a translation.
</p>
</div>
