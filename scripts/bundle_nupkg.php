<?php
foreach (file("src/lua.h") as $line)
{
	if (substr($line, 0, 29) == '#define PLUTO_VERSION "Pluto ')
	{
		$pluto_version = substr(rtrim($line), 29, -1);
		break;
	}
}
$pluto_version or die("Failed to determine Pluto version");

mkdir("plutolang");
copy("LICENSE", "plutolang/LICENSE.txt");
copy("src/pluto.exe", "plutolang/pluto.exe");
copy("src/plutoc.exe", "plutolang/plutoc.exe");
chdir("plutolang");
file_put_contents(".chocogen.json", <<<EOC
{
    "path": ["pluto.exe", "plutoc.exe", "LICENSE.txt", "VERIFICATION.txt"],
    "title": "Pluto Language",
    "description": "A superset of Lua 5.4 — with unique features, optimizations, and improvements.",
    "authors": "Ryan Starrett, Sainan",
    "website": "https://pluto-lang.org/",
    "repository": "https://github.com/PlutoLang/Pluto",
    "tags": "pluto lua development programming foss cross-platform non-admin",
    "icon": "https://pluto-lang.org/img/logo.png",
    "license": "https://github.com/PlutoLang/Pluto/blob/main/LICENSE",
    "changelog": "https://github.com/PlutoLang/Pluto/releases",
    "issues": "https://github.com/PlutoLang/Pluto/issues"
}
EOC);
file_put_contents("VERIFICATION.txt", "These are the same files as in the Windows.zip for this release of Pluto: https://github.com/PlutoLang/Pluto/releases");
file_put_contents("chocogen.php", file_get_contents("https://raw.githubusercontent.com/calamity-inc/chocogen/0b1c843eaa167caf590bd56f46f5a57f6ffc6000/chocogen.php"));
passthru("php chocogen.php $pluto_version");
