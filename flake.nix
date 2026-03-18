{
  description = "A very basic flake";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        tex = pkgs.texlive.combine {
          inherit (pkgs.texlive)
            # Core
            scheme-basic
            latexmk

            # Fonts
            collection-fontsrecommended
            xcharter
            mlmodern

            # Layout
            geometry
            setspace
            titlesec
            tocloft
            enumitem

            # Math
            amsmath
            mathtools

            # Graphics
            graphics
            caption
            float
            booktabs

            # Algorithms
            algorithms
            algorithmicx
            algpseudocodex

            # References
            cite
            bibtex
            csquotes

            # Links
            hyperref

            # Utilities
            xstring
            xcolor
            xkeyval
            mweights
            fontaxes
            microtype
            ;
        };
        buildTex =
          src:
          pkgs.stdenvNoCC.mkDerivation {
            name = "${src}-tex-document";
            inherit src;

            buildInputs = with pkgs; [
              coreutils
              tex
            ];

            phases = [
              "unpackPhase"
              "buildPhase"
              "installPhase"
            ];

            buildPhase = ''
              runHook preBuild

              latexmk -C
              latexmk -pdf -bibtex -file-line-error -halt-on-error main.tex

              runHook postBuild
            '';

            installPhase = ''
              runHook preInstall

              mkdir -p $out/${src}
              cp -r . $out/${src}

              runHook postInstall
            '';
          };
      in
      {
        formatter = pkgs.nixfmt-tree;

        apps.watchdog = {
          type = "app";
          program = "${pkgs.writeShellScript "watchdog" ''
            target="''${1:-proposal}"
            exec ${pkgs.watchexec}/bin/watchexec \
              --exts tex,bib,cls,sty \
              --watch . \
              -- nix build .#"$target"
          ''}";
        };

        packages = {
          empty = buildTex ./empty;
          proposal = buildTex ./proposal;
          ieee = buildTex ./ieee;
        };

        devShells.default = pkgs.mkShell {
          buildInputs = [ tex ];
        };
      }
    );
}
