{
  description = "A superset of Lua 5.4 with a focus on general-purpose programming";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    
    flake-utils.url = "github:numtide/flake-utils";

    flake-compat.url = "github:edolstra/flake-compat";
    flake-compat.flake = false;
  };

  outputs = inputs@{ ... }:
    inputs.flake-utils.lib.eachDefaultSystem
      (system:
        let
          pkgs = import inputs.nixpkgs { inherit system; };
        in
        rec {
          packages = rec {
            plutolang = pkgs.stdenv.mkDerivation {
              pname = "plutolang";
              version = "0.9.4";
              src = ./.;

              nativeBuildInputs = with pkgs; [php patchelf];
              buildInputs = with pkgs; [clang lld];

              buildPhase = ''
                php scripts/compile.php clang++
                for x in static shared pluto plutoc; do
                  php scripts/link_$x.php clang++
                done

                runHook postBuild
              '';
              postBuild = ''
                patchelf --add-rpath ${pkgs.stdenv.cc.cc.lib}/lib src/pluto{,c} src/libpluto.so
              '';
              checkPhase = ''
                src/pluto testes/_driver.pluto
              '';
              installPhase = ''
                install -Dm755 -t $out/bin src/pluto{,c}
                install -Dm644 -t $out/lib src/libpluto{static.a,.so}
                install -Dm644 -t $out/include src/{lua,lauxlib,lualib}.h src/lua.hpp
              '';
            };
            default = plutolang;
          };

          apps = rec {
            pluto = {
              type = "app";
              program = "${packages.plutolang}/bin/pluto";  
            };
            plutoc = {
              type = "app";
              program = "${packages.plutolang}/bin/plutoc";
            };
            default = pluto;
          };

          devShells.default = pkgs.mkShell {
            packages = with pkgs; [
              clang
              lld
              php
            ];
          };
        }
      );
}
