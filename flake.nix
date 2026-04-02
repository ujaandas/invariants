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
        project = pkgs.stdenv.mkDerivation {
          pname = "invariants";
          version = "0.1.0";
          src = ./.;
          sourceRoot = "lang";
          meta.mainProgram = "hello_world";

          nativeBuildInputs = [
            pkgs.cmake
            pkgs.ninja
          ];

          buildInputs = [
            pkgs.gtest
          ];
        };
      in
      {
        packages.default = project;

        checks.default = project;

        apps.default = {
          type = "app";
          program = "${project}/bin/hello_world";
        };

        formatter = pkgs.nixfmt;

        devShells.default = pkgs.mkShell {
          packages = with pkgs; [
            clang-tools
            cmake
            cppcheck
            gtest
            ninja
            nixfmt
            prek
          ];
        };
      }
    );
}
