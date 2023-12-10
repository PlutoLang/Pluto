<?php
require __DIR__."/common.php";
check_compiler();

build_soup();

for_each_obj(function($file)
{
	global $compiler;
	run_command_async($compiler." -o int/{$file}.o -c src/{$file}.cpp");
});
await_commands();
