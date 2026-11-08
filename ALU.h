#ifndef ALU_H
#define ALU_H

#include <cstdint>

class ALU {
public:

    static uint8_t addTwosComplement(uint8_t a, uint8_t b);
    static uint8_t addFloatingPoint(uint8_t a, uint8_t b);
    static uint8_t bitwiseAnd(uint8_t a, uint8_t b);
    static uint8_t bitwiseOr(uint8_t a, uint8_t b);
    static uint8_t bitwiseXor(uint8_t a, uint8_t b);
    static uint8_t rotateRight(uint8_t value, int steps);
};

#endif