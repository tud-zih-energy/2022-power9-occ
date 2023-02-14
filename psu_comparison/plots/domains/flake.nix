{
  description = "R sankey plots";

  outputs = { self, nixpkgs }:
    {
      packages.x86_64-linux =
        let
          pkgs = import nixpkgs { system = "x86_64-linux"; };
          selfpkgs = self.packages.x86_64-linux;
        in {
          ggsankey = with pkgs.rPackages; buildRPackage rec {
            name = "ggsankey";
            version = "be08dd0f86eaee9f9ff9e7ff95d47930660a3c36";
            src = pkgs.fetchzip {
              url = "https://github.com/davidsjoberg/${name}/archive/${version}.zip";
              sha256 = "sha256-au0l6usS8glAHO20pSSN0I6LX73z8nQLviExjJuvlyk=";
            };
            propagatedBuildInputs = [dplyr ggplot2 stringr tidyr];
            nativeBuildInputs = [rlang];
          };

          ggsankey-env = pkgs.rWrapper.override {packages = with pkgs.rPackages; [
            dplyr ggplot2 tikzDevice
            selfpkgs.ggsankey
          ];};
        };

      defaultPackage.x86_64-linux = self.packages.x86_64-linux.ggsankey-env;
    };
}
