# OunceIdent

A port of [joel16/SwitchIdent](https://github.com/joel16/SwitchIdent) to the Ounce (Switch 2 homebrew) devkit. It shows system and hardware information, built against [libounce](https://github.com/ouncebrew/libounce) (libnx for Switch 2) instead of upstream libnx.

**Status: experimental.** The code builds against libounce's libnx-compatible headers, but not every service call is confirmed to work on Switch 2 hardware. Values that fail to read may be blank or wrong.

## Build

Install devkitPro pacman, then the [ounce-devkit](https://github.com/ouncebrew/ounce-devkit) (`sh install.sh`), plus `switch-sdl2 switch-sdl2_image switch-sdl2_ttf`. Then run `make`. The output is `OunceIdent.nro`.

Every push also builds the NRO with GitHub Actions; download it from the run's artifacts (`OunceIdent-nro`).

## Credits
- joel16 and the SwitchIdent contributors (shchmue, beta testers, preetisketch for the banner).
- masagrator and the SwitchBrew wiki for documenting service usage.
- devkitPro for devkitA64, and the ouncebrew project for libounce and the devkit.

Licensed under the same license as SwitchIdent (see LICENSE).
