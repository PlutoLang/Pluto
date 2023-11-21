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

mkdir("pluto");
mkdir("pluto/DEBIAN");
file_put_contents("pluto/DEBIAN/control", <<<EOC
Package: pluto
Version: $pluto_version
Section: custom
Priority: optional
Architecture: all
Essential: no
Maintainer: Sainan <sainan@calamity.gg>
Description: A superset of Lua 5.4 — with unique features, optimizations, and improvements.

EOC);
mkdir("pluto/usr");
mkdir("pluto/usr/local");
mkdir("pluto/usr/local/bin");
copy("src/pluto", "pluto/usr/local/bin/pluto");
copy("src/plutoc", "pluto/usr/local/bin/plutoc");
passthru("dpkg-deb --build pluto");
