# Lackey plugin update builder

This script will build the following files for use in maintaining Lackey CCG plugins remotely:
* `updatelist.txt`
* `version.txt`
* `CardImageUrls1.txt` (or similar)

## Requirements
* Your plugin has the following files required already functioning for Lackey:
  * `plugininfo.txt`
  * `pluginpreferences.txt`
* That you have a series of .txt files in the directory `sets`.
* That you have a series of image files referenced by your set .txt files in `sets/setimages/SETNAME`.
* That you may have additional files such as packdefinitions.
* That you are using source control such as github for remote storage.
* The remote storage file locations match. While Lackey allows you to spread the files around, this builder assumes the directory structure will always be the same.
* You have PHP at least 7.4 available. PHP installation instructions are beyond the scope of this README.

## Using this builder
* You must create a PLUGINNAME.ini
  * It will contain the directory your plugin is in.
  * It will contain the URL your files can be found at.
  * It will contain a list of any additional files that should be added to the updatelist.txt.

* php build.php PLUGINNAME.ini
* Check that your changes are good. Then commit them and push them to the remote repo.

## The ini file
```ini
; The name of the plugin which is used in the files.
plugin = example

; The full path of your git checkout.
path = /path/to/example             

; Optional. The name of the card URLs file if you want it to be something other than the default.
urls_file = CardImageUrls1.txt

; The full URl to the raw version of the file that Lackey can read from. 
url = https://raw.githubusercontent.com/merlinofchaos/lackey-example/main

; Any optional files that should be added to the updatelist. If a directory is
; added, all files within that directory will be added. You do not need to
; add 'sets' or 'packs' or any files required for Lackey to function as these
; will be added automatically.
files[] = additional1.txt
files[] = additional2.txt
```

## Using the example repo:
* `git clone https://github.com/merlinofchaos/lackey-example`
* Potentially update the lackey-example.ini to point to where you checked it out.
* `php build.php lackey-example.ini`
