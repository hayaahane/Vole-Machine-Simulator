#include "CPU.h"
#include "Memory.h"
#include "ALU.h"
#include <iostream>
#include <iomanip>

// Constructor
CPU::CPU() {
    reset();
 }

// Reset
void CPU::reset() { 
    for (int i = 0; i < 16; ++i) {
        registers[i] = 0;
    }
    ir = 0;
    decodedOpcode = 0;
    decodedReg = 0;
    decodedOperand = 0;
    pc = 0;
    halted = false;
 }

// Step
void CPU::step(Memory& memory) {
    if (halted) return;

    ir = fetch(memory);

    if (validate(ir)) {
        decode(ir);
        execute(memory);
    }
    else {
        halted = true;
    }
}
// Fetch
uint16_t CPU::fetch(Memory& memory) {
    if (pc > 254) {
        halted = true;
        return 0;
    }

    uint8_t firstByte = memory.read(pc);
    uint8_t secondByte = memory.read(pc + 1);
    uint16_t instruction = (firstByte << 8) | secondByte;
    pc += 2;
    return instruction;
}
// Validate
bool CPU::validate(uint16_t instruction) const {
    uint8_t opcode = (instruction >> 12) & 0xF;
    uint8_t reg = (instruction >> 8) & 0xF;
    uint8_t operand = instruction & 0xFF;

    switch (opcode)
    {
    case 0x1:
      return true;
  
      case 0x2:
        return true;
      
      case 0x3:
        return true;

        case 0x4:
        return reg==0;
      
        case 0x5:
        return true;
    
        case 0x6:
        return true;
  
        case 0x7:
        return true;

        case 0x8:
        return true;
        
        case 0x9:
        return true;
 
        case 0xA:
        return (operand & 0xF0) == 0;

        case 0xB:
        return true;
  
        case 0xC:
        return instruction==0xC000;
   
    default:
        return false;
    }
}

// Decode
void CPU::decode(uint16_t instruction) { 
     decodedOpcode = instruction >> 12;
     decodedReg = (instruction >> 8) & 0xF;
     decodedOperand = instruction & 0xFF;
}

// Execute
void CPU::execute(Memory& memory) {
    switch (decodedOpcode)
    {
    case 0x1:
    registers[decodedReg] = memory.read(decodedOperand);
      break;
  
      case 0x2:
      registers[decodedReg] = decodedOperand;
        break;
      
      case 0x3:
       memory.write(decodedOperand, registers[decodedReg]);
        break;
      
    case 0x4:{
        uint8_t source = decodedOperand >> 4;
        uint8_t dest = decodedOperand & 0xF;
        registers[dest] = registers[source];
        break;
    }
      case 0x5:{
      uint8_t source1 = (decodedOperand >> 4) & 0xF;
      uint8_t source2 = decodedOperand & 0xF;
      registers[decodedReg] = ALU::addTwosComplement(
      registers[source1],
      registers[source2]
      );
        break;
    }
      case 0x6:{
    uint8_t source1 = (decodedOperand >> 4) & 0xF;
    uint8_t source2 = decodedOperand & 0xF;

    registers[decodedReg] = ALU::addFloatingPoint(
        registers[source1],
        registers[source2]
    );
    break;
}
      case 0x7:{
        uint8_t source1 = (decodedOperand >> 4) & 0xF;
        uint8_t source2 = decodedOperand & 0xF;

        registers[decodedReg] = ALU::bitwiseOr(
            registers[source1],
            registers[source2]
        );
        break;
    }

      case 0x8:{
       uint8_t source1 = (decodedOperand >> 4) & 0xF;
       uint8_t source2 = decodedOperand & 0xF;

        registers[decodedReg] = ALU::bitwiseAnd(
        registers[source1],
        registers[source2]
    );
        break;
}
      case 0x9:{
       uint8_t source1 = (decodedOperand >> 4) & 0xF;
       uint8_t source2 = decodedOperand & 0xF;

        registers[decodedReg] = ALU::bitwiseXor(
        registers[source1],
        registers[source2]
    );
        break;
}
      case 0xA: {
       uint8_t steps = decodedOperand & 0xF;

        registers[decodedReg] = ALU::rotateRight(
        registers[decodedReg],
        steps
    );

    break;
}
case 0xB: {
    if (registers[decodedReg] == registers[0]) {
        pc = decodedOperand;
    }
    break;
}
      case 0xC:
        halted = true;
        break;

    default:
        break;
    }

 }

void CPU::setRegister(uint8_t reg, uint8_t value) {
    if (reg < 16) {
        registers[reg] = value;
    }
}

uint8_t CPU::getRegister(int index) const {
    if (index >= 0 && index < 16) {
        return registers[index];
    }
    return 0;
}

void CPU::setPC(uint8_t address) {
    pc = address;
}

uint8_t CPU::getPC() const {
    return pc;
}

uint16_t CPU::getIR() const {
    return ir;
}

bool CPU::isHalted() const {
    return halted;
}

void CPU::printState() const {
    std::cout << "CPU State:\n";
    std::cout << "PC: " << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(pc) << "\n";
    std::cout << "IR: " << std::hex << std::setw(4) << std::setfill('0') << ir << "\n";
    for (int i = 0; i < 16; ++i) {
        std::cout << "R" << i << ": " << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(registers[i]) << "\n";
    }
    std::cout << "Halted: " << (halted ? "Yes" : "No") << "\n";
    std::cout << std::dec;
}
