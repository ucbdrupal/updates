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

// VARIABLES
$DEBUG = true;
$drush = exec('which drush');
$drush_up = 'pm-update --pipe --security-only';
$egrep = exec('which egrep');
$supported_aliases = $_SERVER['HOME'] . '/.drush/supported.aliases.drushrc.php';
$supported_sites = array();

// FUNCTIONS

/*
 * Leverage drush site-list option:

supported.aliases.drushrc.php:

<?php
$aliases['live'] = array(
  'site-list' => array(
    '@pantheon.ucbapps.live', 
    '@pantheon.scholar.live',
    '@pantheon.ucb-cod.live',
//    '@sdfkj',
   )
);

 */

//TODO: check site-list alias against installed aliases.  make sure they have the aliases they need
function check_aliases($aliases) {
  global $drush, $egrep;
  $dot_drush_aliases = doexec($drush . ' sa', $DEBUG);
  $errors = array("alias" => 0, "connection" => 0);

  foreach ($aliases as $a) {
    if (!in_array('@' . $a, $dot_drush_aliases['out'])) {
      msg("Error: Not found in your drush aliases: $a");
      $errors['alias']++;
      continue;
    }
  }
  if ($errors['alias'] > 0) return false;

  foreach ($aliases as $a) {
    $test = doexec($drush . ' @' . $a . ' ' . "status | $egrep \"Drupal version\"");
    if (strpos($test['out'][0], 'Drupal') === false) {
      $errors['connection']++;
      msg("Error: Could not connect to site alias: $a");
    }
  }
  if ($errors['connection'] > 0) return false;

  return true;
}

// MAIN

// Command Line Options
$shortopts = "hs:";
$longopts = ""; // not supported in < php 5.3
$options = getopt($shortopts);
$usage = "
USAGE:

php update.php

Provides a report of pending updates for the drush aliases
listed in the update*.inc files

Optional arguments:

-s drushalias.example   Check just the site specified by this alias

";


foreach (array_keys($options) as $o) {
  switch ($o) {
    case 's':
      $supported_sites = array($options['s']);
      break;
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

if (count($supported_sites) == 0) {
  if (file_exists($supported_aliases) && is_readable($supported_aliases)) {
    require_once($supported_aliases);
  }
  else {
    msg("Error: Please add $supported_aliases and make sure it's readable.");
  }
}



//if (!check_aliases($supported_sites)) exit(1);


/*

**** parse output differently:

explode on ' >> '

[bwood@ucbmbp ~]$ drush @supported -y vget site_name
You are about to execute 'vget site_name' non-interactively (--yes forced) on all of the following targets:
  @supported.live
Continue?  (y/n): y
pantheon.ucbapps.live >> site_name: "University of California Berkeley Drupal App Server"
pantheon.ucb-cod.live >> Drush command terminated abnormally due to  [error]
an unrecoverable error.
pantheon.scholar.live >> site_name: "Berkeley Scholars"
 
 */

//foreach ($supported_sites as $alias) {
$alias = "supported";
$cmd_result['names'] = doexec($drush . ' @' . $alias . ' -y vget site_name', $DEBUG);
//$name = str_replace('site_name: "', '', trim($name['out'][0], '"'));
//$name = str_replace('site_name: "', '', trim($name['out'][1]));

$cmd_result['basics'] = doexec($drush . ' @' . $alias . " -y status | $egrep \"Drupal version|Site URI\"");
$cmd_result['cron'] = doexec($drush . ' @' . $alias . " -y ws --count=1 \"cron\"");
$cmd_result['updates'][] = doexec($drush . ' @' . $alias . ' -y ' . $drush_up);
//}

print_r($cmd_result); exit;

//print_r($cmd_result);
$num_sites_total = count($cmd_result);
$num_sites_with_updates = 0;
ksort($cmd_result);

while(list($k, $v) = each($cmd_result)) {
  if (strpos($k, ' non-interactively (--yes forced) on all of the following targets') === TRUE) continue;
  if (strpos($k, 'Continue?  (y/n):') === TRUE) continue;
  //msg('**** ' . $k . ' ****');
  msg('*****************************');
  foreach($v['basics']['out'] as $out) {
    if (strpos($out, ' non-interactively (--yes forced) on all of the following targets') === FALSE) msg($out);
    if (strpos($out, 'Continue?  (y/n):') === TRUE) continue;
  }
  print "\n";
  print "  Last cron run:\n";
  foreach($v['cron']['out'] as $out) {
    if (strpos($out, ' non-interactively (--yes forced) on all of the following targets') === FALSE) msg($out);
    if (strpos($out, 'Continue?  (y/n):') === TRUE) continue;
  }
  $num_updates_site = count($v['updates'][0]['out']) - 1; //there's always a blank line at the end
  if ($num_updates_site > 0) {
    $num_sites_with_updates++;
    msg("Found $num_updates_site pending update(s):");
    foreach($v['updates'] as $upd) {
      if ($upd['return'] !== 0) msg("Error executing a command", TRUE);
      foreach($upd['out'] as $out) {
        if (strpos($out, ' non-interactively (--yes forced) on all of the following targets') === FALSE) msg($out);
        if (strpos($out, 'Continue?  (y/n):') === TRUE) continue;
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