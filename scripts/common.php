<?php
chdir(__DIR__."/.."); // Ensure working directory is repository root

if(!is_dir("int"))
{
	mkdir("int");
}

function check_compiler()
{
	global $argv, $compiler;

	if(empty($argv[1]))
	{
		die("Syntax: php {$argv[0]} <compiler>");
	}

	$compiler = resolve_installed_program($argv[1]);
	$compiler .= " -std=c++17 -O3";
	if(defined("PHP_WINDOWS_VERSION_MAJOR"))
	{
		$compiler .= " -D _CRT_SECURE_NO_WARNINGS";
	}
	else
	{
		$compiler .= " -fuse-ld=lld -lm -lstdc++ -lstdc++fs";
	}
}

function resolve_installed_program($exe)
{
	if(defined("PHP_WINDOWS_VERSION_MAJOR"))
	{
		return escapeshellarg(system("where ".escapeshellarg($exe)));
	}
	return escapeshellarg(system("which ".escapeshellarg($exe)));
}

function for_each_obj($f)
{
	foreach(["src","src/vendor/Soup"] as $dir)
	{
		foreach(scandir($dir) as $file)
		{
			if(substr($file, -4) == ".cpp")
			{
				$f(substr($file, 0, -4));
			}
		}
	}
}
