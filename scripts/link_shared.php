<?php
require __DIR__."/common.php";
check_compiler();

$cmd = $compiler." -shared -o src/libpluto.so";

for_each_obj(function($file)
{
	if($file != "lua" && $file != "luac")
	{
		global $cmd;
		$cmd .= " int/{$file}.o";
	}
});

passthru($cmd);
