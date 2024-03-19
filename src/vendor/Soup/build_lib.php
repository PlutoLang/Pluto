<?php
require "build_common.php";

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
	$clang .= " -maes -mpclmul -mrdrnd -mrdseed -msha -msse4.1";
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
