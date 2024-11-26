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
	global $argv, $compiler;

	if(empty($argv[1]))
	{
		die("Syntax: php {$argv[0]} <compiler>");
	}

	$compiler = resolve_installed_program($argv[1]);
	for($i = 2; $i != count($argv); ++$i)
	{
		$compiler .= " ".escapeshellarg($argv[$i]);
	}
	$compiler .= " -std=c++17 -O3 -fvisibility=hidden -fno-rtti -ffunction-sections -fdata-sections";
	if(defined("PHP_WINDOWS_VERSION_MAJOR"))
	{
		$compiler .= " -D _CRT_SECURE_NO_WARNINGS";
	}
	else
	{
		$compiler .= " -Wno-unused-command-line-argument -lm -lstdc++ -pthread -ldl";
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

function prepare_link()
{
	global $compiler;
	$compiler .= " -L".__DIR__."/../src/vendor/Soup -lsoup";
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
