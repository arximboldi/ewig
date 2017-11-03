{ nixpkgs ? (import <nixpkgs> {}).fetchFromGitHub {
    owner  = "NixOS";
    repo   = "nixpkgs";
    rev    = "d0d905668c010b65795b57afdf7f0360aac6245b";
    sha256 = "1kqxfmsik1s1jsmim20n5l4kq6wq8743h5h17igfxxbbwwqry88l";
  }}:

with import nixpkgs {};

let
  compiler_pkg = pkgs.${compiler};
  native_compiler = compiler_pkg.isClang == stdenv.cc.isClang;
  deps = import ./nix/deps.nix { inherit nixpkgs; };
in

stdenv.mkDerivation rec {
  name = "ewig-env";
  buildInputs = [
    gcc7
    cmake
    ncurses
    boost
    deps.immer
    deps.scelta
    deps.utfcpp
    deps.lager
  ];
  shellHook = ''
    export EWIG_ROOT=`dirname ${toString ./shell.nix}`
    export PATH=$PATH:"$EWIG_ROOT/build"
  '';
}
