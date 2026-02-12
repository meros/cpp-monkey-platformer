{
  description = "Monkey Platformer - 2D physics platformer game";

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
            !(builtins.elem name [ "build" "result" ".git" "alleg42.dll" ]);
        };
      in {
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "monkey-platformer";
          version = "0.1.0";
          inherit src;

          nativeBuildInputs = [ pkgs.cmake pkgs.makeWrapper ];
          buildInputs = [
            pkgs.sfml_2
            pkgs.box2d_2
            pkgs.libGL
          ];

          # Run unit tests during build
          doCheck = true;
          checkPhase = ''
            ctest --output-on-failure -E 'Visual|Rendering|Physics'
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
            for f in level.txt apa_stand_right.tga apa_stand_left.tga \
                     apa_walk_right_1.tga apa_walk_right_2.tga \
                     apa_walk_left_1.tga apa_walk_left_2.tga \
                     apa_jump_right.tga apa_jump_left.tga \
                     groundtile.tga rope.tga; do
              test -f $out/share/monkey-platformer/data/$f || \
                (echo "FAIL: missing bundled asset: data/$f" && exit 1)
            done
            echo "All bundled assets verified."
          '';

          meta = {
            description = "2D physics platformer game";
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
            echo "Monkey Platformer dev shell"
            echo "  Build:   cmake -B build && cmake --build build"
            echo "  Test:    cd build && ctest --output-on-failure"
            echo "  Run:     cd build && ./monkey_game"
            echo "  QA:      cmake --build build --target cppcheck"
            echo "  Visual:  ./tests/visual_test.sh"
          '';
        };
      });
}
