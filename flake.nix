{
  description = "Leafwind - a physics-driven jungle platformer about a very small monkey";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};

        src = pkgs.lib.cleanSourceWith {
          src = ./.;
          filter = path: type:
            let name = builtins.baseNameOf path; in
            !(builtins.elem name [ "build" "result" ".git" "save.txt" "save.tmp" ]);
        };
      in {
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "leafwind";
          version = "1.0.0";
          inherit src;

          nativeBuildInputs = [ pkgs.cmake pkgs.makeWrapper ];
          buildInputs = [
            pkgs.sfml_2
            pkgs.box2d_2
            pkgs.libGL
          ];

          # Headless tests run during the build (display-dependent ones need xvfb)
          doCheck = true;
          checkPhase = ''
            ctest --output-on-failure -E 'Visual|Rendering|GameFlow'
          '';

          installPhase = ''
            mkdir -p $out/libexec $out/bin $out/share/monkey-platformer
            cp monkey_game $out/libexec/
            cp -r $src/data $out/share/monkey-platformer/data
            makeWrapper $out/libexec/monkey_game $out/bin/monkey_game \
              --run "cd $out/share/monkey-platformer"
          '';

          # Verify installed package is complete
          doInstallCheck = true;
          installCheckPhase = ''
            for f in apa_stand_right.tga apa_stand_left.tga \
                     apa_walk_right_1.tga apa_walk_right_2.tga \
                     apa_walk_left_1.tga apa_walk_left_2.tga \
                     apa_jump_right.tga apa_jump_left.tga \
                     fonts/DejaVuSans.ttf fonts/DejaVuSans-Bold.ttf fonts/LICENSE-DejaVu.txt \
                     levels/level01.txt levels/level02.txt levels/level03.txt \
                     levels/level04.txt levels/level05.txt levels/level06.txt \
                     levels/level07.txt levels/level08.txt levels/level09.txt \
                     levels/level10.txt; do
              test -f $out/share/monkey-platformer/data/$f || \
                (echo "FAIL: missing bundled asset: data/$f" && exit 1)
            done
            (cd $out/share/monkey-platformer && $out/libexec/monkey_game --validate-levels)
            echo "All bundled assets verified."
          '';

          meta = {
            description = "Leafwind: a physics-driven jungle platformer";
            mainProgram = "monkey_game";
          };
        };

        checks.default = self.packages.${system}.default;

        devShells.default = pkgs.mkShell {
          inputsFrom = [ self.packages.${system}.default ];
          packages = with pkgs; [
            # QA tools
            cppcheck
            clang-tools

            # Visual regression testing
            xvfb-run
            imagemagick
            bc
          ];

          shellHook = ''
            echo "Leafwind dev shell"
            echo "  Build:   cmake -B build && cmake --build build"
            echo "  Test:    cd build && ctest --output-on-failure"
            echo "  Levels:  ./build/monkey_game --validate-levels"
            echo "  QA:      cmake --build build --target cppcheck"
            echo "  Visual:  ./tests/visual_test.sh build/monkey_game"
            echo "  Play:    ./build/monkey_game [--play N]"
          '';
        };
      });
}
