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
  $cmd_result[$alias][] = doexec($drush . ' @' . $alias . ' vget site_name');
  $cmd_result[$alias][] = doexec($drush . ' @' . $alias . ' ' . "status | $egrep \"Drupal version|Site URI\"");
  $cmd_result[$alias][] = doexec($drush . ' @' . $alias . ' ' . $drush_up);
}

while(list($k, $v) = each($cmd_result)) {
  foreach($v as $cmd ) {
    if ($cmd['return'] !== 0) msg("Error executing a command", TRUE);
    foreach($cmd['out'] as $out) {
      msg($out);
    }
  }
  if ($set != $k) print "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n\n";
  $set = $k;
}

?>