{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    utils.url = "github:numtide/flake-utils";
  };
  outputs =
    {
      self,
      nixpkgs,
      utils,
    }:
    utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        sourcePaths = "lang/src lang/tests";

        dev-configure = pkgs.writeShellApplication {
          name = "dev-configure";
          meta.description = "Configure clangd environment.";
          runtimeInputs = with pkgs; [
            clang
            cmake
            ninja
          ];
          text = ''
            set -euo pipefail
            cmake -S lang -B .nix-dev/build
          '';
        };

        invariants = pkgs.stdenv.mkDerivation {
          pname = "invariants";
          version = "0.1.0";
          src = ./lang;
          meta = {
            mainProgram = "hello_world";
            description = "Constrained LLM generation via semantic invariants and refinement types.";
          };

          nativeBuildInputs = with pkgs; [
            cmake
            ninja
          ];

          buildInputs = with pkgs; [
            gtest
          ];

          doCheck = true;
          checkPhase = ''
            ctest --output-on-failure
          '';
        };
      in
      {
        packages = {
          default = invariants;
        };

        checks = {
          default = invariants;
        };

        apps = {
          default = (utils.lib.mkApp { drv = invariants; }) // {
            meta.description = "Run the default invariants executable.";
          };

          configure = {
            type = "app";
            program = "${dev-configure}/bin/dev-configure";
            meta.description = "Configure the local CMake build directory.";
          };
        };

        devShells.default = pkgs.mkShell.override { stdenv = pkgs.clangStdenv; } {
          packages = with pkgs; [
            clang-tools
            cmake
            cppcheck
            include-what-you-use
            gtest
            ninja
            nixfmt
            prek
          ];
        };

        formatter = pkgs.nixfmt;
      }
    );
}
