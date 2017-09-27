with import <nixpkgs> {};

stdenv.mkDerivation rec {
  name = "ewig-git";
  version = "git";
  src = builtins.filterSource (path: type:
            baseNameOf path != ".git" &&
            baseNameOf path != "result" &&
            baseNameOf path != "build")
          ./.;
  nativeBuildInputs = [ cmake ];
  buildInputs = [ gcc7 ncurses boost ];
  meta = with stdenv.lib; {
    homepage    = "https://github.com/arximboldi/ewig";
    description = "The eternal text editor";
    license     = licenses.gpl3;
  };
}
