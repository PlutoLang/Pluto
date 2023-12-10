<?php
require "build_config.php";

$cd = getcwd();
chdir(__DIR__);

// Setup folders
if(!is_dir(__DIR__."/bin"))
{
	mkdir(__DIR__."/bin");
}
if(!is_dir(__DIR__."/bin/int"))
{
	mkdir(__DIR__."/bin/int");
}

// Utilities
$procs = [];

function run_command_async($cmd)
{
	global $procs;
	echo ".";
	$file = tmpfile();
	$descriptorspec = array(
		0 => array("pipe", "r"),
		1 => array("file", stream_get_meta_data($file)["uri"], "a"),
		2 => array("file", stream_get_meta_data($file)["uri"], "a"),
	);
	$proc = proc_open($cmd, $descriptorspec, $pipes);
	array_push($procs, [ $proc, $file ]);
	if (count($procs) > 32)
	{
		await_commands();
	}
}

function await_commands()
{
	global $procs;
	echo "\r";
	$output = "";
	while (count($procs) != 0)
	{
		foreach ($procs as $i => $proc)
		{
			if (!proc_get_status($proc[0])["running"])
			{
				echo "█";
				$output .= stream_get_contents($proc[1]);
				fclose($proc[1]);
				proc_close($proc[0]);
				unset($procs[$i]);
			}
		}
		usleep(50000);
	}
	echo "\n";
	echo $output;
}

echo "Compiling...\n";
$files = [];
$objects = [];
foreach(scandir(__DIR__."/soup") as $file)
{
	if(substr($file, -4) == ".cpp")
	{
		$file = substr($file, 0, -4);
		run_command_async("$clang -c ".__DIR__."/soup/$file.cpp -o ".__DIR__."/bin/int/$file.o");
		array_push($objects, escapeshellarg("bin/int/$file.o"));
	}
}
if(is_dir(__DIR__."/Intrin"))
{
	$clang .= " -maes -mpclmul -msha -msse4.1";
	foreach(scandir(__DIR__."/Intrin") as $file)
	{
		if(substr($file, -4) == ".cpp")
		{
			$file = substr($file, 0, -4);
			run_command_async("$clang -c ".__DIR__."/Intrin/$file.cpp -o ".__DIR__."/bin/int/$file.o");
			array_push($objects, escapeshellarg("bin/int/$file.o"));
		}
	}
}
await_commands();

echo "Bundling static lib...\n";
$archiver = "ar";
$libname = "libsoup.a";
if (defined("PHP_WINDOWS_VERSION_MAJOR"))
{
	$archiver = "llvm-ar";
	$libname = "soup.lib";
}
passthru("$archiver rc $libname ".join(" ", $objects));

chdir($cd);
