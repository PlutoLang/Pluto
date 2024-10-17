<?php
require __DIR__."/common.php";
check_compiler();

prepare_link();
$cmd = $compiler." -o src/pluto";
if(defined("PHP_WINDOWS_VERSION_MAJOR"))
{
	$cmd .= ".exe";
}
else if (PHP_OS_FAMILY != "Darwin")
{
	$cmd .= " -Wl,--export-dynamic";
}
for_each_obj(function($file)
{
	if($file != "luac")
	{
		global $cmd;
		$cmd .= " int/{$file}.o";
	}
});
passthru($cmd);
