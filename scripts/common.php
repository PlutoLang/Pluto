<?php
// Check argument
if(empty($argv[1]))
{
	die("Syntax: php {$argv[0]} <compiler>");
}

// Set $compiler based on argument
if(defined("PHP_WINDOWS_VERSION_MAJOR"))
{
	$compiler = escapeshellarg(system("where ".escapeshellarg($argv[1])));
	$compiler .= " -D _CRT_SECURE_NO_WARNINGS";
}
else
{
	$compiler = escapeshellarg(system("which ".escapeshellarg($argv[1])));
}
$compiler .= " -std=c++17";

// Ensure working directory is repository root
chdir(__DIR__."/..");

function for_each_obj($f)
{
	foreach(scandir("src") as $file)
	{
		if(substr($file, -4) == ".cpp")
		{
			$f(substr($file, 0, -4));
		}
	}
}
