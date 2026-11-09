#ifndef CPU_H
#define CPU_H

#include <cstdint>

class Memory;

class CPU {
private:
    uint8_t registers[16];
    uint16_t ir;
    uint8_t decodedOpcode;
    uint8_t decodedReg;
    uint8_t decodedOperand;
    uint8_t pc;
    bool halted;

public:
    CPU();
    void reset();
    void step(Memory& memory);
    uint16_t fetch(Memory& memory);
    bool validate(uint16_t instruction) const;
    void decode(uint16_t instruction);
    void execute(Memory& memory);
    void setRegister(uint8_t reg, uint8_t value);
    uint8_t getRegister(int index) const;
    void setPC(uint8_t address);
    uint8_t getPC() const;
    uint16_t getIR() const;
    bool isHalted() const;
    void printState() const;
    void resetCPU() {
        pc = 0;
        ir = 0;
        halted = false;
        for(int i = 0; i < 16; i++) {
            registers[i] = 0;
        }
    }

};
#endif
