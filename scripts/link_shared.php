<?php
require __DIR__."/common.php";
check_compiler();

prepare_link();
$cmd = $compiler." -shared -o src/libplutoso.so";

for_each_obj(function($file)
{
	if($file != "lua" && $file != "luac")
	{
		global $cmd;
		$cmd .= " int/{$file}.o";
	}
});

passthru($cmd);
