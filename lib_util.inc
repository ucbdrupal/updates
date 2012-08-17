<?php
//LOG
//FIXME: $notify_address should be passed in
$LOG = array();
$LOGLEVEL = '0';
$ERROR = FALSE;
/**
 *Log Levels:
 *  Notice = 0; // A recoverable problem/issue.
 *  Status = 1; // A routine status message.
 *  Error  = 2; // An Unrecoverable error.
 */
$LOGTYPE = '0'; // 0 print; 1 log file; 2 email
//$DATE = date('Y-m-d H:i:s');
$DATE = null;

$DIR = '/usr/local/backup';
$FILES = array(); // the files used during the backup.

/*** Aegir specific variables ***/
$aegir_user='aegir';
$aegir_home='/var/aegir';
$drush_sql_conf = "sql conf"; #drush 0.3 will be 'sql-conf'
$script_path = '/usr/local/sbin/ucb';

function usage() {
  global $usage;
  print $usage;
}

function msg ($msg, $notice = false) {
  if ($notice) {
    $msg = "==> $msg";
  }
  print "$msg\n";
}

function error ($msg) {
 print "ERROR: $msg\n";
 exit(1);
}

function notify($msg, $level = 0) {
  global $LOG, $ERROR;
  $LOG[] = array('message' => $msg, 'level' => $level);
  if($level == 2) {
    $ERROR = true;
  }
  if ($level = 0) {
    $stdout = fopen('php://stdout', 'w');
    fwrite($stdout, $msg . "\n");
  }

}


//TODO: should be 2 funcitons writelog and emaillog.  write should be called aas part of every update incase the script aborts.
/**
 *function MakeLog
 *  notes:
 *    This function reports the log back to the user, either via
 *    cmdline or email.  Right now it only does cmdline, a flag
 *    and email support will be added soon.
 *  params:
 *    none.
 */
function make_log() {
  global $LOG, $LOGLEVEL, $DATE, $hostname;
  $prefix = array('NOTICE: ', 'STATUS: ', 'ERROR:  ');
  $log = '';
  $notice_count = 0;
  $status_count = 0;
  $error_count = 0;

  foreach($LOG as $line) {
    if($line['level'] >= $LOGLEVEL) {
      $log .= $prefix[$line['level']] . $line['message'] . "\n";
      if($line['level'] == 0) {
        $notice_count++;
      }
      elseif($line['level'] == 1) {
        $status_count++;
      }
      elseif($line['level'] == 2) {
        $error_count++;
      }
    }
  }
  // Prep email
  $subject = '';
  if($error_count == 0) {
    $subject = "SUCCESS - $hostname Backup: $DATE - No errors.";
  }
  else {
    $subject = "ERROR - $hostname Backup: $DATE - $error_count error(s).";
  }
  exec('du -hc', $disk_use);

  $summary = "Backup Summary (Log Level = $LOGLEVEL):\n\n" .
             "$error_count error(s).\n" .
			 "$notice_count notice(s).\n" .
			 "$status_count status(es).\n\n" .
			 "Disk Usage:\n";

  foreach($disk_use as $dfile) {
    $summary .= $dfile . "\n";
  }
  $summary .= "\n\n";

  mail('stafforg-tech@lists.berkeley.edu', $subject, $summary . $log);

  print $subject . "\n\n" . $summary;
  print($log);

}

function get_latest_drupal_core($drupal_core_base, $drupal_version) {
  $files = array();
  $sql_files = array();
  $files = scandir($drupal_core_base);
  $max = null;
  foreach ($files as $f) {
    if (is_dir($drupal_core_base . '/' . $f) === true) {
      if (($f == '.') || ($f == '..')) continue;
      $pos = strpos($f, $drupal_version . '.');
      //'5.' should be found at the 0th position
      if ($pos === 0) {
        $core_dirs[] = $f;
      }
    }
  }
  sort($core_dirs, SORT_NUMERIC);
  $max  = array_pop($core_dirs);
  return $max;
}

/**
 * Get a password from the shell.
 * http://www.dasprids.de/blog/2008/08/22/getting-a-password-hidden-from-stdin-with-php-cli
 *
 * This function works on *nix systems only and requires shell_exec and stty.
 *
 * @param  boolean $stars Wether or not to output stars for given characters
 * @return string
 */
function getPassword($stars = false)
{
  // Get current style
  $oldStyle = shell_exec('stty -g');

  if ($stars === false) {
    shell_exec('stty -echo');
    $password = rtrim(fgets(STDIN), "\n");
  } else {
    shell_exec('stty -icanon -echo min 1 time 0');

    $password = '';
    while (true) {
      $char = fgetc(STDIN);

      if ($char === "\n") {
        break;
      } else if (ord($char) === 127) {
        if (strlen($password) > 0) {
          fwrite(STDOUT, "\x08 \x08");
          $password = substr($password, 0, -1);
        }
      } else {
        fwrite(STDOUT, "*");
        $password .= $char;
      }
    }
  }
  fwrite(STDOUT, "\n");
  // Reset old style
  shell_exec('stty ' . $oldStyle);

  // Return the password
  return $password;
}

function prompt_get_stdin($prompt, $default = null) {
  if (!empty($default)) {
    $prompt .= " ([Enter] for \"$default\")";
  }
  fwrite(STDOUT, $prompt . ": ");
//  $input = strtolower(rtrim(fgets(STDIN), "\n"));
//  $input = strtolower(rtrim(fgets(STDIN), NEWLINE));
  $input = strtolower(rtrim(fgets(STDIN), PHP_EOL));  
  if (($default !== null) && ($input === "")) {
    $input = $default;
  }
  return $input;
}

function prompt_yn($prompt, $default) {
  if (stripos(prompt_get_stdin($prompt, $default), 'n') === 0) {
    return false;
  }
  return true;
}

function prompt_choice_list($choices=array(), $prompt_msg, $default=null) {
  //TODO: vs. fwrite(stdout...) ?
  $default_select = '';
  print $prompt_msg . ":\n";
  while (list($k, $v) = each($choices)) {
    print "\t" . $k . ") " . $v ."\n";
  }
  if (($default !== null) && is_int($default)) {
    $default_select = $default;
  }
  //TODO: allow default to be string mathing $v
  //if only one choice, it will be default
  if (count($choices) == 1) {
    $default_select = 0;
  }
  $choice_prompt = "Enter file number";
  if (is_int($default_select) === true) {
    $choice_prompt .= ' [' . $default_select . ']';
  }
//  $choice_prompt .= ": ";
  $index = prompt_get_stdin($choice_prompt, $default_select);
  return $choices[$index];
}

function doexec($cmd, $verbose=false, $ssh=array()) {
  //Case statement woulda been nicer...
  if (count($ssh) == 2) {
    $ssh_user_host = $ssh[0].'@'.$ssh[1];
    $cmd='ssh -o PasswordAuthentication=no "'.$ssh_user_host.'" "'.$cmd.'"';
    //print $cmd;
  } elseif (count($ssh) == 0) {
    //continue...
  } else {
    msg("ERROR: doexec: Unsupported number of ssh values.");
    exit(1);
  }
  exec($cmd, $cmd_output, $cmd_return); 
  $msg = "";
  // If return is anything besides 0, there's been an error.
  if($cmd_return != 0) {
    if ($verbose) $msg = msg("\tERROR: running command: '$cmd'");
    $pass = false;
  }
  else {
    $pass = true;
    if ($verbose) $msg = msg("\tCommand success: '$cmd'");
  }
  #return $return = array("result" => $pass, "out" => $cmd_output, "msg" => $msg);
  return $return = array("result" => $pass, "out" => $cmd_output, "return" => $cmd_return);
}

function sql_data_file() {
  global $site_dir;
  $site_dir_db = $site_dir . '/database';
  $files = array();
  $sql_files = array();
  $files = scandir($site_dir_db);
  foreach ($files as $f) {
    if (strstr($f, '.sql') !== false) {
      $sql_files[] = $f;
    }
  }
  $data_file = prompt_choice_list($sql_files, "Here are the *.sql files in " . $site_dir_db);
  //TODO: skip create database if exists in dump
  return $site_dir_db . '/' .  $data_file;
}

function parse_mysql_url($db_url) {
  if (preg_match('/^mysql:\/\/(.*):(.*)@(.*)\/(.*)$/',
  $db_url,
  $matches)) {
  		$db_conf['username'] = $matches[1];
  		$db_conf['password'] = $matches[2];
  		$db_conf['hostname'] = $matches[3];
  		$db_conf['database'] = $matches[4];
  }
  return $db_conf;
}

function file_str_replace ($strings, $replacements, $file) {
  $contents = file_get_contents($file);
  $new_contents = str_replace($strings, $replacements, $contents);
  $return = file_put_contents($file, $new_contents);
  return $return;
}
/**
 * Connect to mysql.  Prompt for a password or read from a file.
 * Reading from file helpful for Eclipse debugging since can't read STDIN, afaict
 * 
 * @param string $hostname
 * @param string $username
 * @param string $database (optional)
 * @param string $file (optional)
 * @return resource | false
 */
function mysql_connect_prompt_or_file ($hostname, $username, $database=null, $file=null) {
  global $db_file;
  $conn_id = false;
  if ($db_file) {
    $password = null;
    if (file_exists($file) &&  is_readable($file)) {
      $handle = fopen($file, "r");
      if ($handle) {
        $password = trim(fread($handle, filesize($file)));
        fclose($handle);
      }
      @chmod($file, "0600");
    } 
    else {
      notify("Password file ($file) doesn't exist or is not readable.", 2);
      return $conn_id;
    }
  }
  else {
    fwrite(STDOUT, "MySQL $username password on $hostname: ");
    $password = getPassword(true);
  }
  $conn_id = mysql_connect($hostname, $username, $password);
  //if ($handle) fclose($handle);
  return $conn_id;
}
/**
 * mysql_query_status
 *
 * @param string $sql
 * @param int $error_level for notify if sql fails (default 1)
 * @return boolean
 */
function mysql_query_status($sql, $error_level_fail=1, $mysql_error=false) {
  $success = false;
  if (mysql_query($sql)) {
    notify("Success:\n$sql\n\n", 1);
    $success = true;
  } else {
    notify("FAIL:\n$sql\n\n", $error_level_fail);
  }
  return $success;
}

function mysql_connect_settings_php() {
  //TODO:  parse settings php and connect as site user. see update_domain.php
}

function create_dirs() {
  //FIXME: should take "dir/dir1/dir2" and create that. take perms. return success bool.
  /*
  global $home_dir, $workspace, $subdomain, $logs;
  //TODO: Test success
  //returns false if parent exists
  mkdir($home_dir . '/' . $logs, 0755, true);
  //FIXME: can files perms be locked down based on webserver user/group?
  mkdir($home_dir . '/' . $workspace . '/' . $subdomain . '/files', 0777, true);
  mkdir($home_dir . '/' . $workspace . '/' . $subdomain . '/modules', 0755, true);
  mkdir($home_dir . '/' . $workspace . '/' . $subdomain . '/themes', 0755, true);
  mkdir($home_dir . '/' . $workspace . '/' . $subdomain . '/database', 0755, true);
  */

}

function random_password($length = 12) {

  $chars = "abcdefghijkmnopqrstuvwxyz023456789"; //no @ in mysql pws!
  srand((double)microtime()*1000000);
  $i = 1;
  $pass = '' ;
  while ($i <= $length) {
    $num = rand() % 34;
    $tmp = substr($chars, $num, 1);
    $pass = $pass . $tmp;
    $i++;
  }
  return $pass;
}

/**
 * Find projects and versions in our respository
 * Assumes this dir structure under $dir:
 *
 *      admin_menu/6.x-1.3/admin_menu.module
 *
 * @param unknown_type $dir
 * @return array() of same structure as D6 update.module get_package_info
 *
 *      $found['admin_menu']['info']['version'] = 6.x-1.1
 */
function repository_get_projects($dir) {
  $found = array();
  //TODO make function
  //remove trailing slash if exists
  if (substr($dir, -1) == '/') {
    $dir = substr($dir, 0, strlen($dir) -1);
  }
  if ($handle = opendir($dir)) {
    while (false !== ($file = readdir($handle))) {
      if ($file == '.' || $file == '..' || $file == '.svn') continue;
      if (is_dir($dir . '/' . $file)) {
        $dir2 = $dir . '/' . $file;
        if ($handle2 = opendir($dir2)) {
          while (false !== ($file2 = readdir($handle2))) {
            if ($file2 == '.' || $file2 == '..' || $file2 == '.svn') continue;
            if (is_dir($dir2 . '/' . $file2)) {
              $found[$file]['info']['version'] = $file2;
              //FIXME: (minor) this name might not be quite what update_get_projects has
              $found[$file]['info']['name'] = str_replace('_', ' ', $file);
            }
          }
        }
      }
    }
  }
  return $found;
}

function parse_sqlconf ($site_url, $site_platform=false, $host=false, $cols=true) {
  global $aegir_user, $drush_sql_conf;
  $cmd = "/var/aegir/drush/drush $drush_sql_conf -l " . $site_url;
  // Prevent drush complaint: "tput: No value for $TERM and no -T specified"
  if ($cols) $cmd = 'export COLUMNS=80; '.$cmd; 
  if ($site_platform !== false) {
    $cmd .= " --root=".$site_platform;
  }
  if ($host === false) {
    $sqlconf = doexec($cmd, false);
  } else {
    $ssh = array($aegir_user, $host);
    $sqlconf = doexec($cmd, false, $ssh);
  }
  $tmp = "";
    while (list(,$v) = each($sqlconf["out"])) {
      if (strpos($v, '[')) {
        preg_match('/\[(\w+)\]/', trim($v), $matches);
        $key = $matches[1];
        preg_match('/=>\ {0,1}([^,]{0,})/', trim($v), $matches ); 
        $value = $matches[1];
        $site_db[$key] = $value;    
      }
    }
  return $site_db;
}

function sendmail($to, $from, $subject, $message) {
  $headers = 'From: '. $from . "\r\n" .
  'Reply-To: '. $from . "\r\n" .
  'X-Mailer: PHP/' . phpversion();
  return mail($to, $subject, $message, $headers);
}

/* determines the OS it's running on and sets appropriate line ending constant */
//function set_os_features () {
//  if (strtoupper(substr(php_uname('s'), 0, 3)) === 'WIN') {    
//    define('NEWLINE', "\r\n");
//  }
//  else {
//    define('NEWLINE', "\n");
//  }
//}

?>