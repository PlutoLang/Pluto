<?php
$clang = "clang -std=c++17 -fno-rtti -DSOUP_USE_INTRIN";
if (defined("PHP_WINDOWS_VERSION_MAJOR"))
{
	$clang .= " -D_CRT_SECURE_NO_WARNINGS";
}

$clanglink = $clang;
if (!defined("PHP_WINDOWS_VERSION_MAJOR"))
{
	$clanglink .= " -lstdc++ -pthread -lresolv -lm -ldl";
	if (PHP_OS_FAMILY != "Darwin")
	{
		$clanglink .= " -fuse-ld=lld -lstdc++fs";
	}
}
