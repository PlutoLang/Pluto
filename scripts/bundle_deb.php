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

$arch = trim(shell_exec("dpkg --print-architecture"));

@mkdir("pluto");
@mkdir("pluto/DEBIAN");
@mkdir("pluto/usr");
@mkdir("pluto/usr/bin");
@mkdir("pluto/usr/lib");
@mkdir("pluto/usr/include");
@mkdir("pluto/usr/include/pluto");

file_put_contents("pluto/DEBIAN/control", <<<EOC
Package: pluto
Version: $pluto_version
Section: custom
Priority: optional
Architecture: $arch
Essential: no
Maintainer: Sainan <sainan@calamity.inc>
Description: A superset of Lua 5.4 with a focus on general-purpose programming.

EOC);
chmod("pluto/DEBIAN/control", 0644);

copy("src/pluto", "pluto/usr/bin/pluto");
copy("src/plutoc", "pluto/usr/bin/plutoc");
chmod("pluto/usr/bin/pluto", 0755);
chmod("pluto/usr/bin/plutoc", 0755);

//copy("src/libplutostatic.a", "pluto/usr/lib/libplutostatic.a");
copy("src/libpluto.so", "pluto/usr/lib/libpluto.so");

copy("src/lua.h", "pluto/usr/include/pluto/lua.h");
copy("src/lua.hpp", "pluto/usr/include/pluto/lua.hpp");
copy("src/lualib.h", "pluto/usr/include/pluto/lualib.h");
copy("src/lauxlib.h", "pluto/usr/include/pluto/lauxlib.h");
copy("src/luaconf.h", "pluto/usr/include/pluto/luaconf.h");

passthru("dpkg-deb --build pluto");
