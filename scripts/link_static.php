<?php
require __DIR__."/common.php";

$cmd = $compiler." -static -o src/libpluto.a";

for_each_obj(function($file)
{
	if($file != "lua")
	{
		global $cmd;
		$cmd .= " int/{$file}.o";
	}
});

passthru($cmd);
