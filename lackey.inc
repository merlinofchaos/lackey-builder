<?php

ini_set('auto_detect_line_endings', '1');

/**
 * @param $name
 *
 * @return \LackeyGenerator|null
 */
function lackey_get_generator($name) {
  if (!file_exists($name)) {
    print "Unable to find $name\n";
    exit;
  }
  $ini = parse_ini_file($name);
  if (!$ini) {
    return NULL;
  }

  return new LackeyGenerator($ini);
}

class LackeyGenerator {

  // These required files must always exist.
  public $updatelist_files = [
    'plugininfo.txt',
    'pluginpreferences.txt',
    'version.txt',
    'packs',
    'sets',
  ];

  public $plugin = NULL;
  public $path = NULL;
  public $url = NULL;
  public $urls_file = NULL;

  /**
   * Public constructor.
   */
  public function __construct($settings) {
    foreach (['plugin', 'path', 'url'] as $key) {
      if (empty($settings[$key])) {
        print "Missing key $key from ini file.\n";
        exit(1);
      }

      $this->{$key} = $settings[$key];
    }

    $this->path = str_replace('~/', $_SERVER['HOME'] . '/', $this->path);

    // Make sure path contains trailing '/'
    if (substr($this->path, -1) != '/') {
      $this->path .= '/';
    }

    if (!is_dir($this->path)) {
      print "$this->path is not a directory.\n";
      exit(1);
    }

    // Make sure url doesn't contain trailing '/'
    if (substr($this->url, -1) == '/') {
      $this->url = substr($this->url, 0, -1);
    }

    if (!empty($settings['files']) && is_array($settings['files'])) {
      $this->updatelist_files = array_merge($this->updatelist_files, $settings['files']);
    }

    $this->urls_file = $settings['urls_file'] ?? 'CardImageUrls1.txt';
  }

  /**
   * Create a Lackey plugin checksum for a file, for use in updatelist.txt.
   * @param $filename
   *
   * @return int
   */
  public function generateFileHash($filename) {
    // If we get a file in the form of plugins/ then it's from updatelist.txt and
    // we can automatically translate that to where it really lives.
    if (strpos($filename, 'plugins/') === 0) {
      $filename = str_replace('plugins/' . $this->plugin . '/', $this->path, $filename);
    }

    $fp = fopen($filename, 'rb');
    if (!$fp) {
      return 0;
    }

    $f2 = 0;
    $sum = 0;

    do {
      $f1 = $f2;
      $char = fgetc($fp);

      $ord = is_string($char) ? ord($char) : -1;
      $ord = $ord > 127 ? -256 + $ord : $ord;

      assert(is_int($ord));
      $f2 = ftell($fp);
      if ($ord !== 10 && $ord !== 13) {
        $sum += $ord;
      }
      $sum %= 100000000;
    } while ($f1 !== $f2);

    fclose($fp);

    return $sum;
  }

  /**
   * Fully generate the plugin for all files that can be generated.
   *
   * @param false $clean
   */
  public function generatePlugin($commit_message, $clean = FALSE) {
    $this->generateCardUrlsTxt();

    // Write version.txt
    $this->generateVersionTxt($commit_message);

    // Generate checksums for updatelist.txt
    $this->generateUpdatelistTxt();
  }

  /**
   * Generates the updatelist.txt file with checksums.
   */
  public function generateUpdatelistTxt() {
    $updatelist_file = $this->path . 'updatelist.txt';
    $updatelist = '';
    // First record is always plugin and the date we generated.
    $updatelist .= $this->plugin . "\t" . date('m-d-y') . PHP_EOL;

    // First go through static files.
    foreach ($this->updatelist_files as $file) {
      $updatelist .= $this->updateListEntry($file);
    }

    $updatelist .= $this->updateListEntry($this->urls_file);
    $updatelist .= "CardURLFiles:\n";
    $updatelist .= $this->urls_file . PHP_EOL;

    print "Writing $updatelist_file\n";
    file_put_contents($updatelist_file, $updatelist);
  }

  /**
   * @param string $filename The raw filename relative to the plugin directory.
   */
  function updateListEntry($filename) {
    // First, see if this is a directory. If it is, process everything
    // within the directory.
    if (!file_exists($this->path . $filename)) {
      // Skip empty files so that 'packs' can be normalized.
      return '';
    }

    // Never traverse into setimages as a directory.
    if ($filename == 'sets/setimages') {
      return '';
    }

    if (is_dir($this->path . $filename)) {
      print "Processing directory $filename\n";
      $files = $this->getSortedFilenames($this->path . $filename, $filename);
      $result = '';
      foreach ($files as $file) {
        $result .= $this->updateListEntry($filename . '/' . $file);
      }
      return $result;
    }

    return 'plugins/' . $this->plugin . '/' . $filename . "\t" .
      $this->url . '/' . $filename . "\t" .
      $this->generateFileHash($this->path . $filename) . PHP_EOL;
  }

  function getSortedFilenames($filename, $basename) {
    $files = [];
    $dir = dir($filename);
    while ($file = $dir->read()) {
      if ($file == '.' || $file == '..') {
        continue;
      }
      $files[] = $file;
    }
    $dir->close();

    usort($files, function ($a, $b) {
      $order = [
        'txt' => 0,
        'xml' => 1,
      ];

      $ext_a = pathinfo($a, PATHINFO_EXTENSION);
      $order_a = $order[$ext_a] ?? 99;
      $ext_b = pathinfo($b, PATHINFO_EXTENSION);
      $order_b = $order[$ext_b] ?? 99;
      if ($order_a == $order_b) {
        return strnatcasecmp($a, $b);
      }

      return $order_a < $order_b ? -1 : 1;
    });

    return $files;
  }

  /**
   * Generates the version.txt file.
   *
   * @param string $commit_message
   */
  public function generateVersionTxt($commit_message) {
    $update_date = date('ymd');
    $version_url = $this->url . '/version.txt';
    $update_url = $this->url . '/updatelist.txt';
    $version = <<<EOL
<version>
<lastupdateYYMMDD>$update_date</lastupdateYYMMDD>
<versionurl>$version_url</versionurl>
<updateurl>$update_url</updateurl>
<message>$commit_message</message>
</version>

EOL;

    //* Update the <lastupdate> section of version.txt.
    $version_file = $this->path . 'version.txt';
    print "Writing $version_file\n";
    file_put_contents($version_file, $version);
  }

  /**
   * Generating the card URLs file.
   * @param $urls
   */
  public function generateCardUrlsTxt() {
    $url_data = "CardImageURLs:\n";
    $urls_file = $this->path . $this->urls_file;

    $files = $this->getSortedFilenames($this->path . 'sets', 'sets');
    foreach ($files as $file) {
      $ext = pathinfo($file, PATHINFO_EXTENSION);
      if ($ext != 'txt') {
        continue;
      }

      print "Reading cards from " . $file . PHP_EOL;
      $input_file = $this->path . 'sets/' . $file;
      $input = fopen($input_file, 'r');

      // Eat the headers.
      $headers = fgetcsv($input, "\t");

      while ($row = fgetcsv($input, 0, "\t")) {
        $image_name = $row[2];
        $fileExtension = pathinfo($image_name, PATHINFO_EXTENSION);
        if (!$fileExtension) {
          $image_name .= '.jpg';
        }

        $url_data .= $row[1] . '/' . $image_name . "\t" . $this->url . '/sets/setimages/' . $row[1] . '/' . $image_name . PHP_EOL;
      }
      $url_data .= PHP_EOL;
      fclose($input);
    }

    print "Writing $urls_file\n";
    file_put_contents($urls_file, $url_data);
  }
}
