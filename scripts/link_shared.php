<?php
require __DIR__."/common.php";
check_compiler();

prepare_link();
if (PHP_OS_FAMILY == "Darwin")
{
	$cmd = $compiler." -dynamiclib -o src/pluto.dylib";
}
else
{
	$cmd = $compiler." -shared -o src/libpluto.so";
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
