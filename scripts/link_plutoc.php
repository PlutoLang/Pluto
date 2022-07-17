<?php
require __DIR__."/common.php";
check_compiler();

$cmd = $compiler." -o src/plutoc";
if(defined("PHP_WINDOWS_VERSION_MAJOR"))
{
	$cmd .= ".exe";
}

for_each_obj(function($file)
{
	if($file != "lua")
	{
		global $cmd;
		$cmd .= " int/{$file}.o";
	}
});

passthru($cmd);
