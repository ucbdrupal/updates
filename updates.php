<?php
/*
 Copyright, GPL, No Warranty

 Copyright 2011 UC Berkeley. Written by Brian Durand Wood. This code
 is distributed under the terms of the GNU General Public License. See
 COPYING.

 No warranty is provided and the author claims no responsibility for
 any disasters caused during the execution of this script.
 */

require_once("lib_util.inc");
include_once("updates_aliases_pantheon_v1.inc");
include_once("updates_aliases_pantheon.inc");
// VARIABLES
$drush = exec('which drush');
$drush_up = 'pm-update --pipe --security-only';
$egrep = exec('which egrep');
// FUNCTIONS

// MAIN

// Command Line Options
$shortopts = "h";
$longopts = ""; // not supported in < php 5.3
$options = getopt($shortopts);
$usage = "
USAGE:

Required arguments:

Optional arguments:

";


foreach (array_keys($options) as $o) {
  switch ($o) {
    /*
     case '':
     $opt[''];
     */
    case 'h':
      print $usage;
      exit(1);
      break;
  }
}

/*
 //Checks/Default settings
 if (!isset($opt[''])) {
 msg("  is required");
 exit(1);
 }
 */
msg("Working...\n");

//TODO: Make sure they have correct drush alias files

$supported_sites = array_merge($supported_sites, $supported_sites_v1);

//TODO: check if cron is running

foreach ($supported_sites as $alias) {
  $name = doexec($drush . ' @' . $alias . ' vget site_name');
  $name = str_replace('site_name: "', '', trim($name['out'][0], '"'));
  $cmd_result[$name]['basics'] = doexec($drush . ' @' . $alias . ' ' . "status | $egrep \"Drupal version|Site URI\"");
  $cmd_result[$name]['updates'][] = doexec($drush . ' @' . $alias . ' ' . $drush_up);
}

//print_r($cmd_result); 
$num_sites_total = count($cmd_result);
$num_sites_with_updates = 0;
ksort($cmd_result);

while(list($k, $v) = each($cmd_result)) {
  msg('**** ' . $k . ' ****');
  foreach($v['basics']['out'] as $out) {
    msg($out);
  }
  print "\n";
  $num_updates_site = count($v['updates'][0]['out']) - 1; //there's always a blank line at the end
  if ($num_updates_site > 0) {
    $num_sites_with_updates++;
    msg("Found $num_updates_site pending update(s):");
    foreach($v['updates'] as $upd) {
      if ($upd['return'] !== 0) msg("Error executing a command", TRUE);
      foreach($upd['out'] as $out) {
        msg($out);
      }
    }
  }
  else {
    msg("No pending updates!");
  }
  if ($set != $k) print "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n\n";
  $set = $k;
}


msg("======================== SUMMARY ========================");
msg("Total sites              : $num_sites_total");
msg("Site with pending updates: $num_sites_with_updates");

?>