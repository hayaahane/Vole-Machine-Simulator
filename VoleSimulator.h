#ifndef VOLESIMULATOR_H
#define VOLESIMULATOR_H

#include "CPU.h"
#include "Memory.h"

class VoleSimulator {
private:
    CPU cpu;
    Memory memory;
    bool running;

    void displayMenu();
    void handleLoadFile();
    void handleManualMemoryEdit();
    void handleManualRegisterEdit();
    void runAll();
    void runStepByStep();
    void displayMachineState() const;

public:
    VoleSimulator();
    void start();
    CPU& getCPU() { return cpu; }
    Memory& getMemory() { return memory; }
};

#endif