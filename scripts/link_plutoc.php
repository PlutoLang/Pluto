<?php
require __DIR__."/common.php";

$cmd = $compiler." -o src/plutoc.exe";

for_each_obj(function($file)
{
	if($file != "luac")
	{
		global $cmd;
		$cmd .= " int/{$file}.o";
	}
});

passthru($cmd);
