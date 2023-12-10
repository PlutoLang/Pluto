<?php
require __DIR__."/common.php";
check_compiler();
$procs = [];
$descriptorspec = array(
	0 => array("pipe", "r"),
	1 => array("pipe", "w"),
	2 => array("pipe", "w"),
);
for_each_obj(function($file)
{
	//echo "$file\n";
	global $compiler, $procs, $descriptorspec;
	$procs[$file] = proc_open($compiler." -o int/{$file}.o -c src/{$file}.cpp", $descriptorspec, $pipes);
});
$any_running = false;
do
{
	usleep(50000);
	$any_running = false;
	foreach ($procs as $proc)
	{
		if (proc_get_status($proc)["running"])
		{
			$any_running = true;
			break;
		}
	}
} while ($any_running);
