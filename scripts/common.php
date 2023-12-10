<?php
chdir(__DIR__."/.."); // Ensure working directory is repository root

if(!is_dir("int"))
{
	mkdir("int");
}
if(!is_dir("int/vendor"))
{
	mkdir("int/vendor");
}
if(!is_dir("int/vendor/Soup"))
{
	mkdir("int/vendor/Soup");
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
		$compiler .= " -Wno-unused-command-line-argument -lm -lstdc++";
		if (PHP_OS_FAMILY != "Darwin")
		{
			$compiler .= " -fPIC -lstdc++fs -fuse-ld=lld";
		}
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
	foreach(["","vendor/Soup/"] as $prefix)
	{
		foreach(scandir("src/".$prefix) as $file)
		{
			if(substr($file, -4) == ".cpp")
			{
				$f($prefix.substr($file, 0, -4));
			}
		}
	}
}

$procs = [];
$descriptorspec = array(
	0 => array("pipe", "r"),
	1 => array("pipe", "w"),
	2 => array("pipe", "w"),
);

function run_command_async($cmd)
{
	global $procs, $descriptorspec;
	echo ".";
	$proc = proc_open($cmd, $descriptorspec, $pipes);
	array_push($procs, [ $proc, $pipes[1], $pipes[2] ]);
}

function await_commands()
{
	global $procs;
	echo "\n";
	$output = "";
	while (count($procs) != 0)
	{
		foreach ($procs as $i => $proc)
		{
			if (!proc_get_status($proc[0])["running"])
			{
				echo "█";
				$output .= stream_get_contents($proc[1]);
				$output .= stream_get_contents($proc[2]);
				unset($procs[$i]);
			}
		}
		usleep(50000);
	}
	echo "\n";
	echo $output;
}
