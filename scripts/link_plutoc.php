<?php
require __DIR__."/common.php";
check_compiler();

$cmd = $compiler." -o src/plutoc.exe";

for_each_obj(function($file)
{
	if($file != "lua")
	{
		global $cmd;
		$cmd .= " int/{$file}.o";
	}
});

passthru($cmd);
