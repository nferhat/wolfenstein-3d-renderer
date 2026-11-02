{ pkgs ? import <nixpkgs> {} }: let
  lib = pkgs.lib;
in pkgs.mkShell rec {
  packages = with pkgs; [
    clang-tools
    clang
    wayland-scanner
    gdb
  ];

  buildInputs = with pkgs; [
    wayland
    libxkbcommon
  ];

  LD_LIBRARY_PATH = lib.makeLibraryPath buildInputs;
}

# vim: sw=2
