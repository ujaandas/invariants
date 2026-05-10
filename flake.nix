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
            gtest
          ];
          text = ''
            set -euo pipefail
            cmake -S lang -B .nix-dev/build
          '';
        };

        dev-test = pkgs.writeShellApplication {
          name = "dev-test";
          meta.description = "Run test suite.";
          runtimeInputs = with pkgs; [
            clang
            cmake
            ninja
            gtest
          ];
          text = ''
            set -euo pipefail
            cmake -S lang -B .nix-dev/build
            cmake --build .nix-dev/build
            ctest --test-dir .nix-dev/build --output-on-failure
          '';
        };

        wasm-configure = pkgs.writeShellApplication {
          name = "wasm-configure";
          meta.description = "Build the browser wasm demo.";
          runtimeInputs = with pkgs; [
            cmake
            emscripten
            ninja
          ];
          text = ''
            set -euo pipefail
            emcmake cmake -S wasm -B .nix-dev/wasm -G Ninja -DCMAKE_BUILD_TYPE=Release
            cmake --build .nix-dev/wasm
          '';
        };

        wasm-serve = pkgs.writeShellApplication {
          name = "wasm-serve";
          meta.description = "Serve the browser wasm demo locally.";
          runtimeInputs = with pkgs; [
            cmake
            emscripten
            ninja
            python3
          ];
          text = ''
            set -euo pipefail
            emcmake cmake -S wasm -B .nix-dev/wasm -G Ninja -DCMAKE_BUILD_TYPE=Release
            cmake --build .nix-dev/wasm

            cd .nix-dev/wasm
            python3 -m http.server 8080
          '';
        };

        build-site = pkgs.writeShellApplication {
          name = "build-site";
          meta.description = "Build the sites doc with oojsite.";
          text = ''
            nix run github:ujaandas/oojsite  -- --postDir="docs" --pageDir="site/pages" --staticDir="site/static" --templateDir="site/templates" --componentDir="site/components"
          '';
        };

        build-site-demo = pkgs.writeShellApplication {
          name = "build-site-demo";
          meta.description = "Build the sites doc in devmode with oojsite.";
          text = ''
            nix run github:ujaandas/oojsite  -- --postDir="docs" --pageDir="site/pages" --staticDir="site/static" --templateDir="site/templates" --componentDir="site/components" --dev
          '';
        };

        invariants = pkgs.clangStdenv.mkDerivation {
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

          wasm-configure = {
            type = "app";
            program = "${wasm-configure}/bin/wasm-configure";
            meta.description = "Build the browser wasm demo.";
          };

          wasm-serve = {
            type = "app";
            program = "${wasm-serve}/bin/wasm-serve";
            meta.description = "Build and serve the browser wasm demo.";
          };

          build-site = {
            type = "app";
            program = "${build-site}/bin/build-site";
            meta.description = "Build the site for Invariants.";
          };

          build-site-demo = {
            type = "app";
            program = "${build-site-demo}/bin/build-site-demo";
            meta.description = "Build and serve the site for Invariants.";
          };

          test = {
            type = "app";
            program = "${dev-test}/bin/dev-test";
            meta.description = "Run test suites.";
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
            emscripten
          ];
        };

        formatter = pkgs.nixfmt;
      }
    );
}
