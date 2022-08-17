{ nixpkgs ? (import <nixpkgs> {}).fetchFromGitHub {
    owner  = "NixOS";
    repo   = "nixpkgs";
    rev    = "2eaadfa5be7ce653a2bca31671c406a2ee7f7b0f";
    sha256 = "1sh3338rp2nnyjqpfd2jj9qdl03cd5qy3qqpkgmlf16njzd6bs4y";
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
    deps.zug
  ];
  shellHook = ''
    export EWIG_ROOT=`dirname ${toString ./shell.nix}`
    export PATH=$PATH:"$EWIG_ROOT/build"
  '';
}
