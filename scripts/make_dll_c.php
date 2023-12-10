<?php
require __DIR__."/common.php";
check_compiler();

for_each_obj(function($file)
{
	global $compiler;
	run_command_async($compiler." -o int/{$file}.o -c src/{$file}.cpp -D LUA_BUILD_AS_DLL -D PLUTO_C_LINKAGE=true");
});
await_commands();

$cmd = $compiler." -shared -o src/pluto.dll";
for_each_obj(function($file)
{
	if($file != "lua" && $file != "luac")
	{
		global $cmd;
		$cmd .= " int/{$file}.o";
	}
});
passthru($cmd);
