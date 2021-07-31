#include <cstdlib>
#include <iostream>
#include <ctime>
#include <experimental/random>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include "engine.h"

using std::cout; using std::endl;
using std::string;
using std::basic_filebuf;
using std::printf;
using std::hex;
using std::vector;
using std::rand; using std::srand;
using std::time;
using namespace eng;

int main() {
    Chip8 c8;
    bool orig = true, inc = false;

    // random seed with current time
    srand(time(nullptr));

    // load fonts into memory
    for (auto i = 0; i < 0x50; i++) {
        c8.memory[0x50 + i] = c8.fonts[i];
    }

    // read file into rom array
    const char *file_name = "programs/ibm.ch8";
    // Reading size of file
    FILE * file = fopen(file_name, "rb");
    if (file == NULL) return 0;
    fseek(file, 0, SEEK_END);
    long int size = ftell(file);
    fclose(file);
    // Reading data to array of unsigned chars
    file = fopen(file_name, "rb");
    unsigned char *rom = (unsigned char *) malloc(size);
    fread(rom, sizeof(unsigned char), size, file);
    fclose(file);

    // load rom into memory
    for (auto i = 0x0; i < size; i++) {
        c8.memory[0x200 + i] = rom[i];
    }

    // interpreter loop
    while (true) {
        // fetch instruction
        uint16_t instr = c8.memory[c8.pc] << 8;
        instr |= c8.memory[c8.pc + 1];
        c8.pc += 2;
        
        // nibble information
        uint8_t op = (instr >> 0xC) & 0xF;
        uint8_t x = (instr >> 0x8) & 0xF;
        uint8_t y = (instr >> 0x4) & 0xF;
        uint8_t n = instr & 0xF;
        uint8_t nn = instr & 0xFF;
        uint16_t nnn = instr & 0xFFF;

        // decode and execute instruction
        switch(op) {
            case 0x0:
                switch(nn) {
                    case 0x00:
                        // end of program
                        return 0;
                        break;
                    case 0xE0:
                        // clear the screen
                        for (auto i = 0; i < 0x20; i++) {
                            c8.memory[i] = 0x0;
                        }
                        break;
                    case 0xEE:
                        // pop stack
                        c8.pc = c8.stk.top();
                        c8.stk.pop();
                        break;
                }
                break;
            case 0x1:
                // jump to nnn
                if (c8.pc - 2 == nnn) {
                    // loops on last instruction
                    return 0;
                }
                c8.pc = nnn;
                break;
            case 0x2:
                // call nnn, push current pc to stack
                c8.stk.push(c8.pc);
                c8.pc = nnn;
                break;
            case 0x3:
                // skip if vx is nn
                if (c8.reg[x] == n) {
                    c8.pc += 2;
                }
                break;
            case 0x4:
                // skip if vx is not nn
                if (c8.reg[x] != nn) {
                    c8.pc += 2;
                }
                break;
            case 0x5:
                // skip if vx is vy
                if (c8.reg[x] == c8.reg[y]) {
                    c8.pc += 2;
                }
                break;
            case 0x9:
                // skip if vx is not vy
                if (c8.reg[x] != c8.reg[y]) {
                    c8.pc += 2;
                }
                break;
            case 0x6:
                // set
                c8.reg[x] = nn;
                break;
            case 0x7:
                // add
                c8.reg[x] += nn;
                break;
            case 0x8:
                // logic
                switch(n) {
                    case 0x0:
                        // set
                        c8.reg[x] = c8.reg[y];
                        break;
                    case 0x1:
                        // or
                        c8.reg[x] |= c8.reg[y];
                        break;
                    case 0x2:
                        // and
                        c8.reg[x] &= c8.reg[y];
                        break;
                    case 0x3:
                        // xor
                        c8.reg[x] ^= c8.reg[y];
                        break;
                    case 0x4:
                        // add with carry
                        c8.reg[x] += c8.reg[y];
                        if (((int) c8.reg[x]) + c8.reg[y] > 0xFF) {
                            c8.reg[0xF] = 0x0;
                        } else {
                            c8.reg[0xF] = 0x1;
                        }
                        break;
                    case 0x5:
                        // subtract
                        if (c8.reg[x] < c8.reg[y]) {
                            c8.reg[0xF] = 0x0;
                        } else {
                            c8.reg[0xF] = 0x1;
                        }
                        c8.reg[x] -= c8.reg[y];
                        break;
                    case 0x7:
                        // subtract other way
                        if (c8.reg[y] < c8.reg[x]) {
                            c8.reg[0xF] = 0x0;
                        } else {
                            c8.reg[0xF] = 0x1;
                        }
                        c8.reg[y] -= c8.reg[x];
                        break;
                    case 0x6:
                        if (orig) {
                            c8.reg[x] = c8.reg[y];
                        }
                        c8.reg[0xF] = c8.reg[x] & 0x1;
                        c8.reg[x] >>= 1;
                        break;
                    case 0xE:
                        if (orig) {
                            c8.reg[x] = c8.reg[y];
                        }
                        c8.reg[0xF] = c8.reg[x] >> 7;
                        c8.reg[x] <<= 1;
                        break;
                }
                break;
            case 0xA:
                // set index
                c8.index = nnn;
                break;
            case 0xB:
                // offset jump
                if (orig) {
                    c8.pc = nnn + c8.reg[0];
                } else {
                    c8.pc = nnn + c8.reg[x];
                }
                break;
            case 0xC:
                // random
                c8.reg[x] = ((unsigned char) rand()) & nn;
                break;
            case 0xD:
                // display
                // not implemented yet
                break;
            case 0xE:
                // key press instructions
                // not implemented yet
                if (nn == 0x9E) {
                    ;
                } else if (nn == 0xA1) {
                    ;
                }
                break;
            case 0xF:
                // miscellaneous
                switch(nn) {
                    case 0x07:
                        // put delay in register
                        c8.reg[x] = c8.delay;
                        break;
                    case 0x15:
                        // load register into delay
                        c8.delay = c8.reg[x];
                        break;
                    case 0x18:
                        // load register into sound
                        c8.sound = c8.reg[x];
                        break;
                    case 0x1E:
                        // add to index
                        c8.index += c8.reg[x];
                        if (c8.index > 0xFFF) {
                            c8.index %= 0x1000;
                            c8.reg[0xF] = 1;
                        }
                        break;
                    case 0x0A:
                        // wait for key to be read
                        // not implemented yet
                        break;
                    case 0x29:
                        // set index to the font character in memory
                        c8.index = 0x50 + c8.reg[x] * 0x5;
                        break;
                    case 0x33:
                        // binary-coded decimal conversion
                        // not implemented yet
                        break;
                    case 0x55:
                        // store registers into memory
                        if (inc) {
                            for (auto i = 0; i < x + 1; i++) {
                                c8.memory[c8.index] = c8.reg[i];
                                c8.index++;
                            }
                        } else {
                            for (auto i = 0; i < x + 1; i++) {
                                c8.memory[c8.index + i] = c8.reg[i];
                            }
                        }
                        break;
                    case 0x65:
                        // load registers from memory
                        if (inc) {
                            for (auto i = 0; i < x + 1; i++) {
                                c8.reg[i] = c8.memory[c8.index];
                                c8.index++;
                            }
                        } else {
                            for (auto i = 0; i < x + 1; i++) {
                                c8.reg[i] = c8.memory[c8.index + i];
                            }
                        }
                        break;
                }
            default:
                // unmapped instruction
                break;
        }
    }

    return 0;
}