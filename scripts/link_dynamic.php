<?php
require __DIR__."/common.php";

$cmd = $compiler." -dynamic -o src/libpluto.dll";

for_each_obj(function($file)
{
	if($file != "lua" && $file != "luac")
	{
		global $cmd;
		$cmd .= " int/{$file}.o";
	}
});

passthru($cmd);
