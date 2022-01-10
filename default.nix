{ nixpkgs ? <nixpkgs> }:

with import nixpkgs {};

let
  deps = import ./nix/deps.nix { inherit nixpkgs; };
in

stdenv.mkDerivation rec {
  name = "ewig-git";
  version = "git";
  src = builtins.filterSource (path: type:
            baseNameOf path != ".git" &&
            baseNameOf path != "result" &&
            baseNameOf path != "build")
          ./.;
  nativeBuildInputs = [ cmake ];
  buildInputs = [
    gcc7
    ncurses
    boost
    deps.immer
    deps.scelta
    deps.utfcpp
    deps.lager
  ];
  meta = with lib; {
    homepage    = "https://github.com/arximboldi/ewig";
    description = "The eternal text editor";
    license     = licenses.gpl3;
  };
}
