#define _POSIX_C_SOURCE 199309L

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "sdl.h"
#include "util.h"

// VM constants
#define START_ADDRESS 0x200
#define FONTSET_SIZE 80
#define FONTSET_START_ADDRESS 0x50
#define VIDEO_WIDTH 64
#define VIDEO_HEIGHT 32

// VM fontset
const uint8_t fontset[FONTSET_SIZE] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

// VM variables
uint8_t registers[16];
uint8_t memory[4096];
uint16_t index;
uint16_t pc;
uint16_t stack[16];
uint8_t sp;
uint8_t delayTimer;
uint8_t soundTimer;
uint8_t keypad[16];
uint32_t video[VIDEO_WIDTH * VIDEO_HEIGHT];
uint16_t opcode;

// Dispatch tables
typedef void (*optable)();
optable table0[0xE + 1];
optable table8[0xE + 1];
optable tableE[0xE + 1];
optable tableF[0x65 + 1];
optable table[0xF + 1];

// Clear display
void OP_00E0() {
	memset(video, 0, sizeof(video));
}

// Return from subroutine
void OP_00EE() {
	sp--;
	pc = stack[sp];
}

// Jump to nnn
void OP_1nnn() {
	uint16_t address = opcode & 0x0FFF;

	pc = address;
}

// Call subroutine at nnn
void OP_2nnn() {
	uint16_t address = opcode & 0x0FFF;

	stack[sp] = pc;
	sp++;
	pc = address;
}

// Skip next instruction if Vx = kk
void OP_3xkk() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	uint8_t byte = opcode & 0x00FF;

	if (registers[Vx] == byte) {
		pc += 2;
	}
}

// Skip next instruction if Vx != kk
void OP_4xkk() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	uint8_t byte = opcode & 0x00FF;

	if (registers[Vx] != byte) {
		pc += 2;
	}
}

// Skip next instruction if Vx = Vy
void OP_5xy0() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	uint8_t Vy = (opcode & 0x00F0) >> 4;

	if (registers[Vx] == registers[Vy]) {
		pc += 2;
	}
}

// Set Vx = kk
void OP_6xkk() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	uint8_t byte = opcode & 0x00FF;

	registers[Vx] = byte;
}

// Set Vx = Vx + kk
void OP_7xkk() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	uint8_t byte = opcode & 0x00FF;

	registers[Vx] += byte;
}

// Set Vx = Vy
void OP_8xy0() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	uint8_t Vy = (opcode & 0x00F0) >> 4;

	registers[Vx] = registers[Vy];
}

// Set Vx = Vx OR Vy
void OP_8xy1() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	uint8_t Vy = (opcode & 0x00F0) >> 4;

	registers[Vx] |= registers[Vy];
}

// Set Vx = Vx AND Vy
void OP_8xy2() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	uint8_t Vy = (opcode & 0x00F0) >> 4;

	registers[Vx] &= registers[Vy];
}

// Set Vx = Vx XOR Vy
void OP_8xy3() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	uint8_t Vy = (opcode & 0x00F0) >> 4;

	registers[Vx] ^= registers[Vy];
}

// Set Vx = Vx + Vy, set VF = carry
void OP_8xy4() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	uint8_t Vy = (opcode & 0x00F0) >> 4;

	uint16_t sum = registers[Vx] + registers[Vy];

	if (sum > 255) {
		registers[0xF] = 1;
	} else {
		registers[0xF] = 0;
	}

	registers[Vx] = sum & 0xFF;
}

// Set Vx = Vx - Vy, set VF = NOT borrow
void OP_8xy5() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	uint8_t Vy = (opcode & 0x00F0) >> 4;

	if (registers[Vx] > registers[Vy]) {
		registers[0xF] = 1;
	} else {
		registers[0xF] = 0;
	}

	registers[Vx] -= registers[Vy];
}

// Shift Vx right by 1
void OP_8xy6() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;

	// Save least significant bit in VF
	registers[0xF] = (registers[Vx] & 0x1);

	registers[Vx] >>= 1;
}

// Set Vx = Vy - Vx, set VF = NOT borrow
void OP_8xy7() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	uint8_t Vy = (opcode & 0x00F0) >> 4;

	if (registers[Vy] > registers[Vx]) {
		registers[0xF] = 1;
	} else {
		registers[0xF] = 0;
	}

	registers[Vx] = registers[Vy] - registers[Vx];
}

// Shift Vx left by 1
void OP_8xyE() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;

	// Save least significant bit in VF
	registers[0xF] = (registers[Vx] & 0x80) >> 7;

	registers[Vx] <<= 1;
}

// Skip next instruction if Vx != Vy
void OP_9xy0() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	uint8_t Vy = (opcode & 0x00F0) >> 4;

	if (registers[Vx] != registers[Vy]) {
		pc += 2;
	}
}

// Set I = nnn
void OP_Annn() {
	uint16_t address = opcode & 0x0FFF;

	index = address;
}

// Jump to location nnn + V0
void OP_Bnnn() {
	uint16_t address = opcode & 0x0FFF;

	pc = registers[0] + address;
}

// Set Vx = random byte AND kk
void OP_Cxkk() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	uint8_t byte = opcode & 0x00FF;

	registers[Vx] = (rand() % 256) & byte;
}

// Display n-byte sprite starting at location I at (Vx, Vy), set VF = collision
void OP_Dxyn() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	uint8_t Vy = (opcode & 0x00F0) >> 4;
	uint8_t height = opcode & 0x000F;

	// Wrap if going beyond screen boundaries
	uint8_t xPos = registers[Vx] % VIDEO_WIDTH;
	uint8_t yPos = registers[Vy] % VIDEO_HEIGHT;

	registers[0xF] = 0;

	for (size_t row = 0; row < height; row++) {
		uint8_t spriteByte = memory[index + row];

		for (size_t col = 0; col < 8; col++) {
			uint8_t spritePixel = spriteByte & (0x80 >> col);
			uint32_t *screenPixel = &video[(yPos + row) * VIDEO_WIDTH + (xPos + col)];

			// if sprite pixel is on
			if (spritePixel) {
				// if sprite pixel is also on then collision
				if (*screenPixel == 0xFFFFFFFF) {
					registers[0xF] = 1;
				}

				// XOR with sprite pixel
				*screenPixel ^= 0xFFFFFFFF;
			}
		}
	}
}

// Skip next instruction if key with value of Vx is pressed
void OP_Ex9E() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;

	uint8_t key = registers[Vx];

	if (keypad[key]) {
		pc += 2;
	}
}

// Skip next instruction if key with value of Vx is not pressed
void OP_ExA1() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;

	uint8_t key = registers[Vx];

	if (!keypad[key]) {
		pc += 2;
	}
}

// Set Vx = delay timer value
void OP_Fx07() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;

	registers[Vx] = delayTimer;
}

// Wait for a key press then store value of key in Vx
void OP_Fx0A() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;

	if (keypad[0]) {
		registers[Vx] = 0;
	} else if (keypad[1]) {
		registers[Vx] = 1;
	} else if (keypad[2]) {
		registers[Vx] = 2;
	} else if (keypad[3]) {
		registers[Vx] = 3;
	} else if (keypad[4]) {
		registers[Vx] = 4;
	} else if (keypad[5]) {
		registers[Vx] = 5;
	} else if (keypad[6]) {
		registers[Vx] = 6;
	} else if (keypad[7]) {
		registers[Vx] = 7;
	} else if (keypad[8]) {
		registers[Vx] = 8;
	} else if (keypad[9]) {
		registers[Vx] = 9;
	} else if (keypad[10]) {
		registers[Vx] = 10;
	} else if (keypad[11]) {
		registers[Vx] = 11;
	} else if (keypad[12]) {
		registers[Vx] = 12;
	} else if (keypad[13]) {
		registers[Vx] = 13;
	} else if (keypad[14]) {
		registers[Vx] = 14;
	} else if (keypad[15]) {
		registers[Vx] = 15;
	} else {
		pc -= 2;
	}
}

// Set delay timer = Vx
void OP_Fx15() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;

	delayTimer = registers[Vx];
}

// Set sound timer = Vx
void OP_Fx18() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;

	soundTimer = registers[Vx];
}

// Set I = I + Vx
void OP_Fx1E() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;

	index += registers[Vx];
}

// Set I = location of sprite for digit Vx
void OP_Fx29() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	uint8_t digit = registers[Vx];

	index = FONTSET_START_ADDRESS + (5 * digit);
}

// Store BCD representation of Vx in location I, I+1, I+2
void OP_Fx33() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	uint8_t value = registers[Vx];

	// Ones-place
	memory[index + 2] = value % 10;
	value /= 10;

	// Tens-place
	memory[index + 1] = value % 10;
	value /= 10;

	// Hundreds-place
	memory[index] = value % 10;
}

// Store registers V0 through Vx into memory starting at I
void OP_Fx55() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;

	for (size_t i = 0; i <= Vx; i++) {
		memory[index + i] = registers[i];
	}
}

// Read registers V0 through Vx from memory starting at I
void OP_Fx65() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;

	for (size_t i = 0; i <= Vx; i++) {
		registers[i] = memory[index + i];
	}
}

// Null opcode
void OP_NULL() {}

// Helper functions for dispatch table
void Table0() {
	table0[opcode & 0x000F]();
}

void Table8() {
	table8[opcode & 0x000F]();
}

void TableE() {
	tableE[opcode & 0x000F]();
}

void TableF() {
	tableF[opcode & 0x00FF]();
}

void init() {
	pc = START_ADDRESS;

	// Load fonts into memory
	for (size_t i = 0; i < FONTSET_SIZE; i++) {
		memory[FONTSET_START_ADDRESS + i] = fontset[i];
	}

	// Set random number seed
	srand(time(NULL));

	// Setup displatch table
	table[0x0] = &Table0;
	table[0x1] = &OP_1nnn;
	table[0x2] = &OP_2nnn;
	table[0x3] = &OP_3xkk;
	table[0x4] = &OP_4xkk;
	table[0x5] = &OP_5xy0;
	table[0x6] = &OP_6xkk;
	table[0x7] = &OP_7xkk;
	table[0x8] = &Table8;
	table[0x9] = &OP_9xy0;
	table[0xA] = &OP_Annn;
	table[0xB] = &OP_Bnnn;
	table[0xC] = &OP_Cxkk;
	table[0xD] = &OP_Dxyn;
	table[0xE] = &TableE;
	table[0xF] = &TableF;

	for (size_t i = 0; i <= 0xE; i++) {
		table0[i] = &OP_NULL;
		table8[i] = &OP_NULL;
		tableE[i] = &OP_NULL;
	}

	table0[0x0] = &OP_00E0;
	table0[0xE] = &OP_00EE;

	table8[0x0] = &OP_8xy0;
	table8[0x1] = &OP_8xy1;
	table8[0x2] = &OP_8xy2;
	table8[0x3] = &OP_8xy3;
	table8[0x4] = &OP_8xy4;
	table8[0x5] = &OP_8xy5;
	table8[0x6] = &OP_8xy6;
	table8[0x7] = &OP_8xy7;
	table8[0xE] = &OP_8xyE;

	tableE[0x1] = &OP_ExA1;
	tableE[0xE] = &OP_Ex9E;

	for (size_t i = 0; i <= 0x65; i++) {
		tableF[i] = &OP_NULL;
	}

	tableF[0x07] = &OP_Fx07;
	tableF[0x0A] = &OP_Fx0A;
	tableF[0x15] = &OP_Fx15;
	tableF[0x18] = &OP_Fx18;
	tableF[0x1E] = &OP_Fx1E;
	tableF[0x29] = &OP_Fx29;
	tableF[0x33] = &OP_Fx33;
	tableF[0x55] = &OP_Fx55;
	tableF[0x65] = &OP_Fx65;
}

// CHIP-8 cycle
void cycle() {
	// Fetch
	opcode = (memory[pc] << 8) | memory[pc + 1];

	pc += 2;

	// Decode and execute
	table[(opcode & 0xF000) >> 12]();

	// Decrement delay timer if it has not been set
	if (delayTimer > 0) {
		delayTimer--;
	}
	// Decrement sound timer if it has not been set
	if (soundTimer > 0) {
		soundTimer--;
	}
}

void loadROM(const char *filename) {
	FILE *fp;
	char ch;
	uint16_t size;

	fp = fopen(filename, "r");

	// Get size of file
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	for (size_t i = 0; i < size; i++) {
		ch = getc(fp);
		memory[START_ADDRESS + i] = ch;
	}

	fclose(fp);
}

int main(int argc, char *argv[]) {
	if (argc < 4) {
		die("usage: c8 [scale] [delay] [ROM file]");
	}

	int videoScale = atoi(argv[1]);
	int cycleDelay = atoi(argv[2]);
	const char *filename = argv[3];

	initSDL("CHIP-8 Emulator", VIDEO_WIDTH * videoScale, VIDEO_HEIGHT * videoScale, VIDEO_WIDTH, VIDEO_HEIGHT);
	init();

	loadROM(filename);

	int videoPitch = sizeof(video[0]) * VIDEO_WIDTH;

	struct timespec timestamp1;
	clock_gettime(CLOCK_MONOTONIC, &timestamp1);
	int quit = 0;

	while (!quit) {
		quit = processInput(keypad);

		struct timespec timestamp2;
		clock_gettime(CLOCK_MONOTONIC, &timestamp2);
		double dt  = (1000.0 * timestamp2.tv_sec + 1e-6 * timestamp2.tv_nsec)
			- (1000.0 * timestamp1.tv_sec + 1e-6 * timestamp1.tv_nsec);

		if (dt > cycleDelay) {
			timestamp1 = timestamp2;

			cycle();

			updateSDL(video, videoPitch);
		}
	}

	return 0;
}
