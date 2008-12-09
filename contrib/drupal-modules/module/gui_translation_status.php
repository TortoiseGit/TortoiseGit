<!--break-->
<?php
//
// Drupal GUI translation status report 
// loaded into:
// "http://tortoisesvn.net/translation_devel_gui" 
//    *AND*
// "http://tortoisesvn.net/translation_release_gui"
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

include("/var/www/vhosts/default/htdocs/modules/tortoisesvn/trans_data_trunk.inc");
include("/var/www/vhosts/default/htdocs/modules/tortoisesvn/trans_countries.inc");
include("/var/www/vhosts/default/htdocs/modules/tortoisesvn/tortoisevars.inc");

// different settings depending on dev or release report
// uncomment these three lines for the trunk report
 $tsvn_var['showold']=TRUE;
 $tsvn_var['reposurl']=$tsvn_var['repos_trunk'].'Languages/';
 $tsvn_var['header_msg']='the current development version of TortoiseSVN, which is always ahead of the latest official release';
// uncomment these three lines for the release branch report
// $tsvn_var['showold']=FALSE;
// $tsvn_var['reposurl']=$tsvn_var['repos_branch'].'Languages/';
// $tsvn_var['header_msg']='the latest official TortoiseSVN release <b>('.$tsvn_var['release'].')</b>';

function print_header($tsvn_var)
{
?>

<div class="content">
<h2>Translations (in Revision <?php echo $tsvn_var['wcrev']; ?>)</h2>

<p>
This page is informing you about the GUI translation status of <?php echo $tsvn_var['header_msg']; ?>.
The statistics are calculated for the HEAD revision and updated regularly.
The last update was run at <b><?php echo $tsvn_var['update']; ?></b>.
</p>

<p>
If you want to download the po file from the repository, either use <strong>guest (no password)</strong> or your tigris.org user ID. 
</p>

<?php
}

//------------------------------------
//
// The program starts here
//

// Merge translation and country information into one array
$TortoiseGUI = array_merge_recursive($countries, $TortoiseGUI);

// Convert Data into a list of columns
foreach ($TortoiseGUI as $key => $row) {
   $potfile[$key] = abs($row[0]);
   $country[$key] = $row[3];
   $errors[$key] = $row[5];
   $total[$key] = $row[6];
   $transl[$key] = $row[7];
   $fuzzy[$key] = $row[8];
   $untrans[$key] = $row[9];
   $accel[$key] = $row[10];
   $name[$key] = $row[11];
   $fdate[$key] = $row[12];
}

// Add $TortoiseGUI as the last parameter, to sort by the common key
array_multisort($potfile, $country, $transl, $untrans, $fuzzy, $accel, $TortoiseGUI);

print_header($tsvn_var);

// Print Alphabetical statistics
print_table_header('sort_alpha', 'Languages ordered by country', $TortoiseGUI['zzz'], $tsvn_var);
print_all_stats($TortoiseGUI, $tsvn_var);
print_table_footer();

array_multisort($potfile, SORT_ASC, $transl, SORT_DESC, $untrans, SORT_ASC, $fuzzy, SORT_ASC, $accel, SORT_ASC, $country, SORT_ASC, $TortoiseGUI);

print_table_header('sort_status', 'Languages by translation status', $TortoiseGUI['zzz'], $tsvn_var);
print_all_stats($TortoiseGUI, $tsvn_var);
print_table_footer();

print_footer($tsvn_var);

?>
