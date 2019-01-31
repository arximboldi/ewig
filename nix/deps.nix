{ nixpkgs ? <nixpkgs>}:

with import nixpkgs {};

rec {
  immer = stdenv.mkDerivation rec {
    name = "immer-${version}";
    version = "git-${commit}";
    commit = "ed8999d81f1c7763705c829deb9122cde19195f4";
    src = fetchFromGitHub {
      owner = "arximboldi";
      repo = "immer";
      rev = commit;
      sha256 = "009iazyjzzh4b5yg7d7y69g4p3gyj8ripgjxcbpyrp92967d45q7";
    };
    nativeBuildInputs = [ cmake ];
    buildInputs = [ boost ];
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

  libhttpserver = stdenv.mkDerivation rec {
    name = "libhttpserver-${version}";
    version = "git-${commit}";
    commit = "4895f43ed29195af70beb47bcfd1ef3ab4555665";
    src = fetchFromGitHub {
      owner = "etr";
      repo = "libhttpserver";
      rev = commit;
      sha256 = "1qg5frqvfhb8bpfiz9wivwjz2icy3si112grv188kgypws58n832";
    };
    propagatedBuildInputs = [ libmicrohttpd ];
    nativeBuildInputs = [ autoreconfHook gcc5 ];
    configureScript = "../configure";
    preConfigure = ''
      substituteInPlace ./configure \
        --replace "/usr/bin/file" "${file}/bin/file" \
        --replace "/bin/pwd" "${coreutils}/bin/pwd"
      mkdir build && cd build
    '';
    meta = with stdenv.lib; {
      homepage = "https://github.com/etr/libhttpserver";
      description = "C++ library for creating an embedded Rest HTTP server (and more)";
      license = licenses.lgpl2;
    };
  };

  cereal = stdenv.mkDerivation rec {
    name = "cereal-${version}";
    version = "git-arximboldi-${commit}";
    commit = "f158a44a3277ec2e1807618e63bcb8e1bd559649";
    src = fetchFromGitHub {
      owner = "arximboldi";
      repo = "cereal";
      rev = commit;
      sha256 = "1zny1k00npz3vrx6bhhdd2gpsy007zjykvmf5af3b3vmvip5p9sm";
    };
    nativeBuildInputs = [ cmake ];
    cmakeFlags="-DJUST_INSTALL_CEREAL=true";
    meta = with stdenv.lib; {
      homepage = "http://uscilab.github.io/cereal";
      description = "A C++11 library for serialization";
      license = licenses.bsd3;
    };
  };

  lager = stdenv.mkDerivation rec {
    name = "lager";
    version = "git-${commit}";
    commit = "bee1c04c058b872ea998421d43587de9e90f079e";
    src = fetchFromGitHub {
      owner = "arximboldi";
      repo = "lager";
      rev = commit;
      sha256 = "1222nydan0vflgfyijvkb1rwgspnbkimp05mm9zwzx6i3hhax4yk";
    };
    buildInputs = [
      ncurses
    ];
    nativeBuildInputs = [
      cmake
      gcc7
      sass
    ];
    propagatedBuildInputs = [
      boost
      immer
      libhttpserver
      cereal
    ];
    meta = with stdenv.lib; {
      homepage    = "https://github.com/arximboldi/lager";
      description = "library for functional interactive c++ programs";
      license     = licenses.lgpl3Plus;
    };
  };
}
