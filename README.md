# c8

Another CHIP-8 emulator

## Building

Make sure that you have `SDL2` installed as well as the development libraries.
Then run:

``` sh
make
```

## Running

To run `c8` you will need to supply a CHIP-8 ROM file (You can find some ROM
files [here](https://github.com/dmatlack/chip8/tree/master/roms/games))

``` sh
./c8 -r rom.ch8
```

Note: Some CHIP-8 emulators in the past had interesting quirks. Some CHIP-8
programs depend on these quirks to function properly. The `-shift`, `-vf`,
`-mem`, and `-jump` flags enable these separate quirks. If your CHIP-8 does
not run or run incorrectly try enabling some of these flags.

## What is CHIP-8?

The CHIP-8 is a virtual machine dating all the way back to the mid-1970s.
It is a very simple VM that has the following specs:

- 4 kilobytes of RAM
- 16 8-bit registers
- Subroutine stack
- Timers for delays and sound
- 16 key keyboard input
- 64x32 pixel monochrome display
- Monotone beeper
- 35 opcodes/instructions

The CHIP-8 isn't really a useful platform, even for games, even though
the majority of CHIP-8 software is just games. It **IS** however, a
great platform to learn how emulation and virtual machines work.

Implementing a CHIP-8 VM is relatively easy on modern systems and in the
process it teaches the user how virtual machines and emulators work.
The CHIP-8 is sort of like a *"Baby's First Virtual Machine"* for someone
who has never dipped their toes into this field.

## TODO

This project is still a work in progress. Some things that need to be
changed are:

- Implement and test other CHIP-8 "quirks"
- Port to other systems(?)

## Note

This specific CHIP-8 implementation is based on code from Austin Morlan.
More specifically, it is based off of
[this article](https://austinmorlan.com/posts/chip8_emulator/).
Needless to say, my version has many different alterations and improvements
when compared to the original.
