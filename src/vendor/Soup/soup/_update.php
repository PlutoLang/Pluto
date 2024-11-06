<?php
// Initialise $dir to folder containing the "vendor" dir
$dir = dirname(dirname(dirname(dirname(__FILE__))));

// Find Soup repository in parent folders
$i = 0;
while (!is_dir("$dir/Soup"))
{
	$dir = dirname($dir);
	if (++$i == 100)
	{
		die("Failed to find Soup repository in parent folders.\n");
	}
}

// Update vendored files
$dir .= "/Soup/soup";
foreach (scandir(".") as $f)
{
	if (substr($f, -4) == ".cpp" || substr($f, -4) == ".hpp")
	{
		if ($f == "base.hpp")
		{
			file_put_contents($f,
				str_replace("\r\n#define NAMESPACE_SOUP namespace soup", "\r\nnamespace soup { namespace pluto_vendored {}; using namespace pluto_vendored; };\r\n#define NAMESPACE_SOUP namespace soup::pluto_vendored",
					str_replace("#define SOUP_EXCAL throw()", "#define SOUP_EXCAL",
						file_get_contents("$dir/$f")
					)
				)
			);
		}
		else
		{
			copy("$dir/$f", $f);
		}
	}
}
