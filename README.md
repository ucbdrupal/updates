Updates
=======

This utility script leverages drush to check all IST Drupal managed sites for module updates. 

Features:

* Sites alphbetized by site_name
* Number of updates pending per site
* Summary info at the bottom

Currently the script assumes that you have two drush alias files 
~/.drush/pantheon.aliases.drushrc.php
~/.drush/v1pantheon.aliases.drushrc.php

Whenever we *launch* a new managed site, you should add its "live" alias to the appropriate update_aliases_*.inc file.
