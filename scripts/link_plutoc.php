<?php
require __DIR__."/common.php";
check_compiler();

$cmd = $compiler." -o src/plutoc";
if($is_windows_target)
{
	$cmd .= ".exe";
}
for_each_obj(function($file)
{
	if($file != "lua")
	{
		global $cmd;
		$cmd .= " int/{$file}.o";
	}
});
$cmd .= get_link_flags();
passthru($cmd);
