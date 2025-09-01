<?php
require __DIR__."/common.php";
check_compiler();

$cmd = $compiler." -o src/pluto";
if($is_windows_target)
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
$cmd .= get_link_flags();
passthru($cmd);
