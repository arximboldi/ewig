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
    meta = with lib; {
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
    meta = with lib; {
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
    meta = with lib; {
      description = "UTF-8 with C++ in a Portable Way";
      license = licenses.mit;
    };
  };

  libhttpserver = stdenv.mkDerivation rec {
    name = "libhttpserver-${version}";
    version = "git-${commit}";
    commit = "36cef5bce3337caa858bc21aa0ca48c33d236b17";
    src = fetchFromGitHub {
      owner = "etr";
      repo = "libhttpserver";
      rev = commit;
      sha256 = "sha256-reGFjtLNUNOiq6wGR5FmAZT6lv1UZyQoS8GxARfDkIY=";
    };
    propagatedBuildInputs = [ libmicrohttpd ];
    nativeBuildInputs = [ autoreconfHook gcc7 ];
    configureScript = "../configure";
    preConfigure = ''
      substituteInPlace ./configure \
        --replace "/usr/bin/file" "${file}/bin/file" \
        --replace "/bin/pwd" "${coreutils}/bin/pwd"
      mkdir build && cd build
      export CFLAGS=-fpermissive CXXFLAGS="-fpermissive -std=c++11"
    '';
    meta = with lib; {
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
    meta = with lib; {
      homepage = "http://uscilab.github.io/cereal";
      description = "A C++11 library for serialization";
      license = licenses.bsd3;
    };
  };

  lager = stdenv.mkDerivation rec {
    name = "lager";
    version = "git-${commit}";
    commit = "56125daacdd2301ab2a8298801d247a593bd4d25";
    src = fetchFromGitHub {
      owner = "arximboldi";
      repo = "lager";
      rev = commit;
      sha256 = "093kw3xahl9lgscbkkx5n6f0mmd0gwa4ra1l34gan1ywhf24kn9v";
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
    meta = with lib; {
      homepage    = "https://github.com/arximboldi/lager";
      description = "library for functional interactive c++ programs";
      license     = licenses.lgpl3Plus;
    };
  };
}
