#include "ALU.h"

uint8_t ALU::addTwosComplement(uint8_t a, uint8_t b) {
    return a + b;
}

uint8_t ALU::addFloatingPoint(uint8_t a, uint8_t b) {
    uint8_t sign_a = (a >> 7) & 1;
    uint8_t exp_a = (a >> 4) & 0x07;
    uint8_t mantissa_a = a & 0x0F;
    uint8_t sign_b = (b >> 7) & 1;
    uint8_t exp_b = (b >> 4) & 0x07;
    uint8_t mantissa_b = b & 0x0F;
    
    if (exp_a > exp_b) {
        mantissa_b >>= (exp_a - exp_b);
        exp_b = exp_a;
    } else if (exp_b > exp_a) {
        mantissa_a >>= (exp_b - exp_a);
        exp_a = exp_b;
    }

    uint8_t mantissa_result = 0;
    uint8_t final_sign = sign_a; 
    uint8_t final_exp = exp_a;


    if (sign_a == sign_b) {
        mantissa_result = mantissa_a + mantissa_b;
        final_sign = sign_a;
        if (mantissa_result > 15){
            mantissa_result >>= 1;
            final_exp = exp_a + 1;
        }
    } 
    else {
        if (mantissa_a >= mantissa_b) {
            mantissa_result = mantissa_a - mantissa_b;
            final_sign = sign_a;
        } else {
            mantissa_result = mantissa_b - mantissa_a;
            final_sign = sign_b;
        }
    }
    return (final_sign << 7) | (final_exp << 4) | (mantissa_result & 0x0F);
}

uint8_t ALU::bitwiseAnd(uint8_t a, uint8_t b) {
    return a & b;
}

uint8_t ALU::bitwiseOr(uint8_t a, uint8_t b) {
    return a | b;
}

uint8_t ALU::bitwiseXor(uint8_t a, uint8_t b) {
    return a ^ b;
}

uint8_t ALU::rotateRight(uint8_t value, int steps) {
    return (value >> steps%8) | (value << (8 - steps%8));
}