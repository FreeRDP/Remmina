#to use in nix, use "nix-build"
let
  pkgs = import <nixpkgs> {};
in
  pkgs.remmina.overrideAttrs (oldAttrs: {
    # Use the source code in the current directory.
    src = ./.;
  })