# Licensing

KAWA is free software under the GNU General Public License, version 3 only (`GPL-3.0-only`, full text in
`LICENSE`). You may use, study, change and share it. If you give someone a firmware built from it, give
them its complete source under the same licence too.

*En bref : KAWA est un logiciel libre sous GPL-3.0. Tu peux l'utiliser, l'étudier, le modifier et le
partager ; si tu donnes un firmware construit à partir de lui, donne aussi son code source.*
*En resumen: KAWA es software libre bajo GPL-3.0. Puedes usarlo, estudiarlo, cambiarlo y compartirlo; si
das un firmware construido a partir de él, da también su código fuente.*

## Who made what

| | |
| --- | --- |
| KAWA: `source/firmware/src/kawa*.c`, `source/firmware/gen/kawa_*.h`, `source/tools/gen_*.py`, `source/tests/`, `source/web/`, `emulator/`, `manual/`, the install launchers and the docs | Omnipotard |
| The live effects of K-GLITCH in `source/firmware/src/kawa_kg.c` (the DESTROY and BEAT banks, the scratch, the FILTER), from K-GLITCH's `kg_dd.c`, `kg_beat.c` and `kg_filter.c`; its trapezoidal state-variable filter after Andrew Simper's (Cytomic) published design | Omnipotard ([K-GLITCH](https://github.com/Guizmoh/K-Glitch)), GPL-3.0-only |
| KAWA's dog: `source/art/boot-dog.png` (the boot picture, the dog in 32 bits) and `source/art/mascot-dog.png` (the 16-bit dog the mascot is drawn after) | Omnipotard |
| The platform: `source/firmware/hal/`, `source/firmware/loader/`, `crt0.S`, `app.ld`, `source/firmware/src/` (`usb.c`, `ota.c`, `recovery.c`, `lcd.c`, `gfx.c`, `libc.c`, `bootguard.h`, `uac_fir.h`), `source/tools/build.py`, `install/fm1_install.py`, `source/tools/fm1pkg_make.py`, `source/tools/lz4blk.py`, `source/tools/get_toolchain.sh` | Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments ([Felucca](https://github.com/hugelton/Felucca)), with the changes of [SLOOP](https://github.com/isod89/sloop-fm1) (isod89) |
| The wiring `source/firmware/src/platform.c` and the host mode that `source/tests/host.c` started from | zvenson ([Dinghy](https://github.com/zvenson/dinghy)) |
| (Gabare) Skiff's synth engines ANALOG, PHASE, LOFI, VOICE, TRIO, WHEEL and PHYS: `source/firmware/src/skiff/` (`dsp.c`, `eng_*.c` but `eng_string.c` and `eng_add.c`, `sk_core.h` and `skiff_tables.h` taken from Skiff's `core.h` and its generated tables), copied unchanged | illuminus5 ([Skiff](https://github.com/illuminus5/skiff)), on Felucca by Leo Kuroshita (Hügelton Instruments), GPL-3.0-only |
| (Gabare) JIANT's engines STRING and ADD: `source/firmware/src/skiff/eng_string.c`, `eng_add.c`, changed to one voice a track (the changes are listed at the top of each file) | Juan Ignacio Antonucci ([JIANT](https://github.com/juanjiant-bit/JIANT)), with Leo Kuroshita (Hügelton Instruments), GPL-3.0-only |
| (Gabare 0.9) SLOOP's NOISE and DIGITAL (FM4): `source/firmware/src/skiff/eng_noise.c`, `eng_digital.c` (their render unchanged, their sounds and description in Skiff's form); the TRS MIDI IN reader `source/firmware/src/midi_uart.c`; the INSERT effects (SOFT .. CHORUS) and the arpeggiator's modes after SLOOP's `fx.c` and `seq.c` | Leo Kuroshita (Hügelton Instruments, Felucca), through [SLOOP](https://github.com/isod89/sloop-fm1) (isod89), GPL-3.0-only |
| (Gabare) What joins them to KAWA (`kawa_sk.c`, `kawa_sk_names.h`, `source/tools/gen_sk_names.py`), the arpeggiator (`kawa_arp.c`), the DX7 bank (in `kawa_edit.c`, `kawa_sysex.c`, `dx7.html`), the PHYS limit, the boot screen and the pages of Gabare | Gabare, GPL-3.0-only |

KAWA changes the platform in two places: `platform.c` widens the flash range it may write to the project
slots (`STORE_LO` to `STORE_HI`) and includes `source/web/wasm_host.c` for the browser build. `source/tools/build.py`
builds `kawa.c` under KAWA's package identity (`FM-1_95N`).

## Sounds

Every sound of KAWA is synthesised. The breaks are made by `source/tools/gen_breaks.py` from sines, noise and
filters, and played in classic break rhythms; no recording is used. The eight drum kits, the waves and the
noise are generated live by the firmware. The sounds you put in the four user slots are yours.

## Third-party material

| What | Licence | Where |
| --- | --- | --- |
| Terminus font 8x16 (ter-u16n), rendered into `source/firmware/gen/felucca_font.h`: the screen font BASE | SIL OFL 1.1 | `source/assets/fonts/Terminus-LICENSE.txt` |
| The screen fonts DOUCE, CLAIRE and 8-BIT (OPTIONS, KNOB 4), rendered into `source/firmware/gen/kawa_fonts.h` by `source/tools/gen_fonts.py`: derived from IBM Plex Mono SemiBold (Copyright © 2017 IBM Corp., Reserved Font Name "Plex"), Atkinson Hyperlegible Mono (Copyright 2020-2024 The Atkinson Hyperlegible Mono Project Authors) and Press Start 2P (Copyright 2012 The Press Start 2P Project Authors, Reserved Font Name "Press Start 2P"). As Modified Versions under the OFL, they do not carry the Reserved Font Names | SIL OFL 1.1 | `source/assets/fonts/` (the fonts and their licences) |
| msfa by Google Inc. and Pascal Gauthier, from Dexed (<https://github.com/asb2m10/dexed>), in Felucca's integer C port (Leo Kuroshita, Hügelton Instruments): the FM track's synthesis. Only msfa is used. Eight of the twelve FM factory sounds (`source/tools/gen_fm_patches.py`) are Felucca's (GPL-3.0); the other four are KAWA's | Apache-2.0 | `source/firmware/src/fm6_core.c`, `source/LICENSES/Apache-2.0-msfa.txt` |
| (Gabare) Physical models of DaisySP (Electrosmith, Emilie Gillet) and Rings' sympathetic strings (Emilie Gillet), in Skiff's fixed-point ports: PHYS's | MIT (the notice is at the top of each file) | `source/firmware/src/skiff/phys_dsp.c`, `phys_symp.c` |
| (Gabare) The FM-1's 128 original voices (M-VAVE firmware V15, by 珠海笙科智能科技有限公司 / M-VAVE), from `app/fm_factory.inc` of <https://github.com/m-vave/fm1_open>, decompressed by `source/tools/gen_mvave.py`, otherwise unchanged: FM SOUND's voices after KAWA's | Apache-2.0 | `source/firmware/gen/kawa_mvave.h`, `source/LICENSES/Apache-2.0-mvave.txt` |
| JieLi AC79 SDK: `uboot.boot`, `cfg_tool.bin`, `eq_cfg_hw.bin` are read from your SDK checkout at build time and placed in the package; no SDK files are in this tree | Apache-2.0 | <https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK> |
| Fonts of the emulator page (Silkscreen, Atkinson Hyperlegible, IBM Plex Mono), loaded from Google Fonts | SIL OFL 1.1 | not in this tree |

## Trademarks

"Felucca" and "Hügelton Instruments" are names of Hügelton Instruments; "M-VAVE" and "FM-1" belong to their
owners. KAWA is not made or endorsed by them. (Gabare) "DX7" and "Yamaha" belong to Yamaha Corporation; Gabare carries
none of Yamaha's sounds (the DX7 banks are the user's) and is not made or endorsed by Yamaha, nor by the authors of
Skiff, JIANT or KAWA.
