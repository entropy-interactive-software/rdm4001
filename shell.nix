#let
#	nixpkgs = fetchTarball "https://github.com/NixOS/nixpkgs/tarball/nixos-25.05";
#	pkgs = import nixpkgs { config = {}; overlay = []; };
#in
with import <nixpkgs> {};

clangStdenv.mkDerivation {
	name = "dev-shell";
	src = null;
  nativeBuildInputs = [
    llvmPackages_21.clang-tools
  ];
	buildInputs = [
    llvmPackages.openmp
		sdl3
		bullet
		libsndfile
		curl
		mpv
		icu
		luajit
		openal
		libtomcrypt
		ninja
		meson
		xz	
		stdenv
		pkg-config
		assimp
		enet
		editline
		mesa
		libGL
		shaderc
		glm
		readline
		xorg.libX11.dev
		qrencode
    freetype
    gdb
	];
}
