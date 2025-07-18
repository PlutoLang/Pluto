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
		run_command_async("$clang -c ".__DIR__."/soup/$file.cpp -o ".__DIR__."/bin/int/$file.o -DSOUP_STANDALONE");
		if ($file != "soup")
		{
			array_push($objects, escapeshellarg("bin/int/$file.o"));
		}
	}
}
await_commands();

echo "Bundling static lib...\n";
$archiver = "ar";
$libname = "libsoup.a";
$dllname = "libsoupbindings.so";
if (defined("PHP_WINDOWS_VERSION_MAJOR"))
{
	$archiver = "llvm-ar";
	$libname = "soup.lib";
	$dllname = "soupbindings.dll";
}
passthru("$archiver rc $libname ".join(" ", $objects));

if (file_exists("bin/int/soup.o"))
{
	echo "Linking shared lib...\n";
	passthru("$clanglink -o $dllname --shared bin/int/soup.o ".join(" ", $objects));
}

chdir($cd);
