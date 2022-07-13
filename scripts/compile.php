<?php
require __DIR__."/common.php";

for_each_obj(function($file)
{
	echo "$file\n";
	global $compiler;
	passthru($compiler." -o int/{$file}.o -c src/{$file}.cpp");
});
