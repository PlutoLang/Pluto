<?php
// Config
$clang = "clang -std=c++17 -fno-rtti -DSOUP_USE_INTRIN";
if (defined("PHP_WINDOWS_VERSION_MAJOR"))
{
	$clang .= " -D_CRT_SECURE_NO_WARNINGS";
}
else if (PHP_OS_FAMILY != "Darwin")
{
	$clang .= " -fPIC";
}

$clanglink = $clang;
if (!defined("PHP_WINDOWS_VERSION_MAJOR"))
{
	$clanglink .= " -lstdc++ -pthread -lm -ldl";
	if (!getenv("ANDROID_ROOT"))
	{
		$clanglink .= " -lresolv";
	}
	if (PHP_OS_FAMILY != "Darwin")
	{
		$clanglink .= " -fuse-ld=lld";
		if (!getenv("ANDROID_ROOT"))
		{
			$clanglink .= " -lstdc++fs";
		}
	}
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
	if (count($procs) >= (getenv("ANDROID_ROOT") ? 16 : 32))
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
