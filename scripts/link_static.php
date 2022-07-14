<?php
require __DIR__."/common.php";

if(defined("PHP_WINDOWS_VERSION_MAJOR"))
{
	$cmd = resolve_installed_program("llvm-ar")." rcu src/libpluto.a";
}
else
{
	$cmd = resolve_installed_program("ar")." rcu src/libpluto.a";
}

for_each_obj(function($file)
{
	if($file != "lua" && $file != "luac")
	{
		global $cmd;
		$cmd .= " int/{$file}.o";
	}
});

passthru($cmd);
