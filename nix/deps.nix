{ nixpkgs ? <nixpkgs>}:

with import nixpkgs {};

rec {
  immer = stdenv.mkDerivation rec {
    name = "immer-${version}";
    version = "git-${commit}";
    commit = "2885357e5fc9fba171c4613f1841591af9e31cf1";
    src = fetchFromGitHub {
      owner = "arximboldi";
      repo = "immer";
      rev = commit;
      sha256 = "0pg6y4n3ibx4axpl1lnjik51mzvr8fyafdaqpc259lljzvi96r05";
    };
    nativeBuildInputs = [ cmake ];
    meta = with stdenv.lib; {
      homepage = "http://sinusoid.es/immer";
      description = "Immutable data structures for C++";
      license = licenses.lgpl3;
    };
  };

  scelta = stdenv.mkDerivation rec {
    name = "scelta-${version}";
    version = "git-${commit}";
    commit = "bd121843e227435f8f55c4aaf7e2de536f05b583";
    src = fetchFromGitHub {
      owner = "SuperV1234";
      repo = "scelta";
      rev = commit;
      sha256 = "0vcc2zh7mn517c8z2p32cg2apixzmx7wwmklzrcdxfk583cxim8c";
    };
    dontBuild = true;
    installPhase = "mkdir -vp $out/include; cp -vr $src/include/* $out/include/";
    meta = with stdenv.lib; {
      description = "Syntax sugar for variants";
      license = licenses.mit;
    };
  };

  utfcpp = stdenv.mkDerivation rec {
    name = "utfcpp-${version}";
    version = "git-${commit}";
    commit = "67036a031d131b5a929a525677e4356850fa4e37";
    src = fetchFromGitHub {
      owner = "nemtrif";
      repo = "utfcpp";
      rev = commit;
      sha256 = "12h4ysmlzqgx19v251bkx839v5fav4qg3hry9a45d1xkl0l1sqsm";
    };
    dontBuild = true;
    installPhase = "mkdir -vp $out/include; cp -vr $src/source/* $out/include/";
    meta = with stdenv.lib; {
      description = "UTF-8 with C++ in a Portable Way";
      license = licenses.mit;
    };
  };
}
