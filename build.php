#!/usr/local/bin/php
<?php

include_once __DIR__ . '/lackey.inc';
if (empty($argv[1])) {
  usage($argv);
}

$generator = lackey_get_generator($argv[1]);
if (empty($generator)) {
  print "Unknown plugin $argv[1]\n";
  exit(1);
}

$commit_message = readline('Provide a version message: ');

$generator->generatePlugin($commit_message, !empty($argv[2]) && $argv[2] == 'clean');
function usage($argv) {
  print "Usage: php $argv[0] /path/to/plugin.ini\n";
  exit;
}
