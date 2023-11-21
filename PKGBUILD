pkgname=pluto
pkgver=tbd
pkgrel=1
arch=('x86_64')
makedepends=('php')
source=("pluto-git::git+https://github.com/PlutoLang/Pluto")
sha256sums=('SKIP')

pkgver () {
	git rev-parse --short HEAD
}

build () {
	cd pluto-git
	php scripts/compile.php clang
	php scripts/link_pluto.php clang
	php scripts/link_plutoc.php clang
}

package () {
	cd $srcdir/pluto-git
	mkdir -p $pkgdir/usr/local/bin
	cp src/pluto $pkgdir/usr/local/bin/pluto
	cp src/plutoc $pkgdir/usr/local/bin/plutoc
}
