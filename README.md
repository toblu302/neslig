# NESlig
A NTSC NES emulator written in C. It was done as an educational project, and is not very usable compared to many other emulators.

### Compiling and running
Use

>make

to compile the emulator, and run

>NESlig [path to iNes file]

to run the emulator.

### Dependencies
* SDL2

### What does it do
You can play a handful of games on it (Super Mario Bros, Mario Bros, Balloon Fight, and Donkey Kong have been tested), assuming the game uses mapper 0 (NROM) and does not have 8x16 sprites. Check out [this list](http://tuxnes.sourceforge.net/nesmapper.txt) to find out which mapper a game uses.

### Accuracy
The emulator is not very accurate, as it emulates on a per-instruction basis instead of per-clock-cycle. It works by first emulating the CPU for a single instruction, and then running the PPU for three times as many clock cycles as the CPU used. The functionality of the CPU instructions have been verified by running blargg's test roms, and are probably the most accurate part of the emulator. The emulator also doesn't emulate every hardware quirk or edge case in the PPU, such as the sprite overflow bug (or even the expected behavior) and the open bus behavior.

### Future features (TODOs)
* More mappers than mapper 0
* 8x16 sprites
* ALU
* Cycle-accuracy
* Multiple controllers