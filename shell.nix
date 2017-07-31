with import <nixpkgs> {};

{ compiler ? "gcc7" }:

let
  compiler_pkg = pkgs.${compiler};
  native_compiler = compiler_pkg.isClang == stdenv.cc.isClang;
in

stdenv.mkDerivation rec {
  name = "ewig-env";
  buildInputs = [
    cmake
    boost
    ncurses
  ] ++ stdenv.lib.optionals compiler_pkg.isClang [libcxx libcxxabi];
  propagatedBuildInputs = stdenv.lib.optional (!native_compiler) compiler_pkg;
  nativeBuildInputs = stdenv.lib.optional native_compiler compiler_pkg;
}
