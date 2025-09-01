<?php
chdir(__DIR__."/.."); // Ensure working directory is repository root

if(!is_dir("int"))
{
	mkdir("int");
}

function build_soup()
{
	global $argv;

	echo ">>> Building Soup\n";
	passthru("php ".__DIR__."/../src/vendor/Soup/build_lib.php ".escapeshellarg($argv[1]));
	echo ">>> Compiling Pluto\n";
}

function check_compiler()
{
	global $argv, $compiler, $is_windows_target;

	if(empty($argv[1]))
	{
		die("Syntax: php {$argv[0]} <compiler>");
	}

	$compiler = resolve_installed_program($argv[1]);
	$is_windows_target = defined("PHP_WINDOWS_VERSION_MAJOR") || stripos($argv[1], "mingw") !== false;
	for($i = 2; $i != count($argv); ++$i)
	{
		$compiler .= " ".escapeshellarg($argv[$i]);
	}
	$compiler .= " -std=c++17 -O3 -fvisibility=hidden -fno-rtti -ffunction-sections -fdata-sections";
	if($is_windows_target)
	{
		$compiler .= " -D _CRT_SECURE_NO_WARNINGS";
		if (!defined("PHP_WINDOWS_VERSION_MAJOR"))
		{
			$compiler .= " -static-libstdc++";
		}
	}
	else
	{
		if (PHP_OS_FAMILY == "Darwin")
		{
			$compiler .= " -lc++";
		}
		else
		{
			$compiler .= " -lstdc++";
		}
		$compiler .= " -Wno-unused-command-line-argument -lm -pthread -ldl";
		if (!getenv("ANDROID_ROOT"))
		{
			$compiler .= " -lresolv";
		}
		if (PHP_OS_FAMILY != "Darwin")
		{
			$compiler .= " -fPIC -fuse-ld=lld -Wl,--gc-sections,--icf=safe";
			if (!getenv("ANDROID_ROOT"))
			{
				$compiler .= " -lstdc++fs";
			}
		}
	}
}

function get_link_flags()
{
	global $is_windows_target;
	$flags = " -L".__DIR__."/../src/vendor/Soup -lsoup";
	if ($is_windows_target)
	{
		$flags .= " -lws2_32 -lbcrypt";
		$flags .= " -municode";
	}
	return $flags;
}

function resolve_installed_program($exe)
{
	if(defined("PHP_WINDOWS_VERSION_MAJOR"))
	{
		return escapeshellarg(system("where ".escapeshellarg($exe)));
	}
	return escapeshellarg(system("command -v ".escapeshellarg($exe)));
}

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
