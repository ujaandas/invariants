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

        dev-configure = pkgs.writeShellApplication {
          name = "dev-configure";
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

        dev-test = pkgs.writeShellApplication {
          name = "dev-test";
          runtimeInputs = with pkgs; [
            cmake
            ninja
          ];
          text = ''
            set -euo pipefail
            if [ ! -f .nix-dev/build/build.ninja ]; then
              ${dev-configure}/bin/dev-configure
            fi
            cmake --build .nix-dev/build
            ctest --test-dir .nix-dev/build --output-on-failure
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
        };
      in
      {
        packages.default = invariants;

        checks = {
          default = invariants;
          tests = invariants;
        };

        apps = {
          default = utils.lib.mkApp { drv = invariants; };

          configure = {
            type = "app";
            program = "${dev-configure}/bin/dev-configure";
            meta.description = "Configure clangd environment.";
          };

          test = {
            type = "app";
            program = "${dev-test}/bin/dev-test";
            meta.description = "Run test suite with Ctest.";
          };
        };

        devShells.default = pkgs.mkShell {
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
