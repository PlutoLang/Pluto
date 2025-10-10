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
			array_push($objects, "bin/int/$file.o");
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
else if (PHP_OS_FAMILY == "Darwin")
{
	$dllname = "libsoupbindings.dylib";
}
passthru("$archiver rc $libname ".join(" ", array_map("escapeshellarg", $objects)));

if (file_exists("bin/int/soup.o"))
{
	echo "Linking shared lib...\n";
	if (PHP_OS_FAMILY == "Darwin")
	{
		passthru("$clanglink -o $dllname -dynamiclib bin/int/soup.o ".join(" ", array_map("escapeshellarg", $objects)));
	}
	else
	{
		passthru("$clanglink -o $dllname --shared bin/int/soup.o ".join(" ", array_map("escapeshellarg", $objects)));
	}
}

// We don't reuse these objects if run again, so might as well clean up.
foreach ($objects as $object)
{
	unlink($object);
}
if (file_exists("bin/int/soup.o"))
{
	unlink("bin/int/soup.o");
}

chdir($cd);
