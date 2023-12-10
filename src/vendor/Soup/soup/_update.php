<?php
// Initialise $dir to folder containing the "vendor" dir
$dir = dirname(dirname(dirname(__FILE__)));

// Find Soup repository in parent folders
$i = 0;
while (!is_dir("$dir/Soup"))
{
	$dir = dirname($dir);
	if (++$i == 100)
	{
		die("Failed to find Soup repository in parent folders.\n");
	}
}

// Update vendored files
$dir .= "/Soup/soup";
foreach (scandir(".") as $f)
{
	if (substr($f, -4) == ".cpp" || substr($f, -4) == ".hpp")
	{
		copy("$dir/$f", $f);
	}
}
