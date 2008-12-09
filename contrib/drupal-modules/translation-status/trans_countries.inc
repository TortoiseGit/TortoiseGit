<?php
//
// List of countries for the Drupal status pages
//
// Copyright (C) 2004-2008 the TortoiseSVN team
// This file is distributed under the same license as TortoiseSVN
//
// $Author: luebbe $
// $Date: 2008-10-30 23:00:14 +0800 (Thu, 30 Oct 2008) $
// $Rev: 14452 $
//
// Author: Lübbe Onken 2004-2008
//

// Meaning of the columns:
// 1: Tag with multiple function
//    po/pot file (pot == 0, po <> 0)
//    Create download link (yes == 1, no <> 1)
//    Also affects sort order on status and download page. 
//    Sorted unsigned, by abs()
//    Recommended usage:
//    0 = pot file -> No download link, shown on translation status page as .po template
//    1 = po file  -> Create download link, shown on translation status page 
//    -1 = po file -> No download link, but shown on translation status page
// 2: Flag byte used to create links on different download pages
//    !!! Bit0 is the rightmost bit !!!
//    Bit0 = Spellchecker available
//    Bit1 = TortoiseSVN manual available
//    Bit2 = TortoiseMerge manual available
// 3: Country code. Used to display the flag. The [flagname].png has to match this column
// 4: Full language name.
// 5: List of translators.
$countries = array(
"bg"    => array("1","001","bg","Bulgarian","Milen A. Radev"),
// "bs_BA" => array("1","000","bs_BA","Bosnian/Herzegovinan","Boris Musa"),
"ca_ES" => array("1","001","ca_ES","Catalan","Arnau Perelló"),
"cs"    => array("1","001","cs","Czech","Radek Svarz, Vojtech Vondra"),
"da"    => array("1","001","da","Danish","Jan Normann Nielsen, Gunther Strube, Christian Funder Sommerlund, David Trasbo, Mikkel Elmholdt"),
"de"    => array("1","111","de","German","Stefan Küng, Lübbe Onken"),
"el"    => array("1","000","el","Greek","George Marselis"),
"eo"    => array("-1","000","eo","Esperanto","Tim Morley"),
"es"    => array("1","111","es","Spanish","Fernando P. Nájera Cano, Antonio Romero Lopez, Franco Catena, Marco Klipfel"),
"fa"    => array("1","000","fa","Persian","Titto Agustine, Mohsen Sajjadi"),
"fr"    => array("1","111","fr","French","Olivier Booklage, Philippe Torche, Jean-Christophe Gigogne, Jérémy Badier, Jean-Christophe Mazenc, Christophe Sauveur, Frédéric Bodin"),
"fi"    => array("1","110","fi","Finnish","Pekka Järvinen, Kari Granö"),
"hr"    => array("1","101","hr","Croatian","Helix d.o.o: Dinko Srkoc, Mario Saric, Boris Musa"),
"hu"    => array("1","001","hu","Hungarian","Szente-Varga Bálint, Halász Gábor"),
"id"    => array("1","111","id","Indonesian","Zaenal Mutaqin, Thomas Edwin Santosa"),
"it"    => array("1","001","it","Italian","Luca Berti, Cecilia Saltori"),
"ja"    => array("1","110","ja","Japanese","Daisuke Deguchi (GUI), Kurasawa Nozomu (docs)"),
"ko"    => array("1","000","ko","Korean","Hojin Choi, Jae-Hong Lee, Kim Taedong"),
"ka"    => array("1","000","ka","Georgian","Gia Shervashidze"),
"mk"    => array("1","000","mk","Macedonian","Srdjan Stokic"),
"nb"    => array("1","001","nb","Norwegian, bokmål","Jostein Andersen, Oddvar Kloster"),
"nl"    => array("1","101","nl","Dutch","Jean-Marc van Leerdam, Yves Reynhout, Bob Hulst"),
"pl"    => array("1","001","pl","Polish","Michal Mielniczuk, Andrzej Rusin, Piotr Kontek, Krzysztof Kotowicz"),
"pt_BR" => array("1","001","pt_BR","Portuguese, Brazil","CRAb Group, Humberto Osório, Emanuele Santos, Guilherme Cantisano, Juliano Ravasi Ferraz, Victor Westmann, Conrado Quilles Gomes, Tiago Deliberali Santos, Leandro Herrera"),
"pt_PT" => array("1","000","pt_PT","Portuguese, Portugal","Manuel Menezes de Sequeira"),
"ro"    => array("1","000","ro","Romanian","Seby Rusu"),
"ru"    => array("1","111","ru","Russian","Dzmitry Lazerka, Dimitry Mardiyan, Shura Zam, Alexander Leschinsky, Stanislav Petrakov"),
"sk"    => array("1","111","sk","Slovak","Oto Brezina"),
"sl"    => array("1","111","sl","Slovenian","Goran Kavrecic, Martin Srebotnjak, Matjaz Cepon"),
"sr_spc"=> array("1","110","sr_spc","Serbian cyrillic","Milorad Popović"),
"sr_spl"=> array("1","110","sr_spl","Serbian latin","Milorad Popović"),
"sv"    => array("1","001","sv","Swedish","Johan Stenqvist, Johan Appelgren"),
"tr"    => array("1","000","tr","Turkish","Aydogan Yolacan, Ali Karaoglu"),
"uk"    => array("1","000","uk","Ukrainian","Vadim Mikhnevych"),
"zh_CN" => array("1","110","zh_CN","Chinese, simplified","Dongsheng Song, Cecil Sheng, Ken Chew, Cobra Net, Xingyu Wang, Lixinfeng"),
"zh_TW" => array("1","000","zh_TW","Chinese, traditional","Cecil Sheng"),
"zzz"   => array("0","111","gb","Template (English)","Stefan Küng, Simon Large, Lübbe Onken")
);
?>
