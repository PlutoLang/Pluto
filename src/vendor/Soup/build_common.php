<?php
// Config
$clang = $argv[1] ?? "clang";
$clang .= " -std=c++17 -fno-rtti -O3 -ffunction-sections -fdata-sections";
if (defined("PHP_WINDOWS_VERSION_MAJOR"))
{
	$clang .= " -D_CRT_SECURE_NO_WARNINGS";
}
else
{
	$clang .= " -fPIC";
}

$clanglink = $clang;
if (!defined("PHP_WINDOWS_VERSION_MAJOR"))
{
	if (PHP_OS_FAMILY == "Darwin")
	{
		$clanglink .= " -lc++ -framework IOKit -framework CoreFoundation";
	}
	else
	{
		$clanglink .= " -lstdc++";
	}
	$clanglink .= " -pthread -lm -ldl";
	if (!getenv("ANDROID_ROOT"))
	{
		$clanglink .= " -lresolv";
	}
	if (PHP_OS_FAMILY != "Darwin")
	{
		$clanglink .= " -fuse-ld=lld -Wl,--gc-sections,--icf=safe";
		if (!getenv("ANDROID_ROOT"))
		{
			$clanglink .= " -lstdc++fs";
		}
	}
}

// Utilities

$queued_commands = [];

function run_command_async($cmd)
{
	global $queued_commands;
	array_push($queued_commands, $cmd);
}

function await_commands()
{
	global $queued_commands;

	$nthreads = 8;
	if (defined("PHP_WINDOWS_VERSION_MAJOR"))
	{
		$nthreads = (int)getenv("NUMBER_OF_PROCESSORS");
	}
	else if (is_file("/proc/cpuinfo"))
	{
		$nthreads = (int)substr_count(file_get_contents("/proc/cpuinfo"), "processor");
	}

	$done = 0;
	$todo = count($queued_commands);
	echo "$done/$todo";

	$procs = [];
	$output = "";
	while ($done != $todo)
	{
		while (count($procs) < $nthreads && count($queued_commands) != 0)
		{
			$file = tmpfile();
			$descriptorspec = array(
				0 => array("pipe", "r"),
				1 => array("file", stream_get_meta_data($file)["uri"], "a"),
				2 => array("file", stream_get_meta_data($file)["uri"], "a"),
			);
			$proc = proc_open(array_shift($queued_commands), $descriptorspec, $pipes);
			array_push($procs, [ $proc, $file ]);
		}

		foreach ($procs as $i => $proc)
		{
			if (!proc_get_status($proc[0])["running"])
			{
				$output .= stream_get_contents($proc[1]);
				fclose($proc[1]);
				proc_close($proc[0]);
				unset($procs[$i]);

				$done += 1;
				echo "\r$done/$todo";
			}
		}

		usleep(10000);
	}

	echo "\n";
	echo $output;
}
