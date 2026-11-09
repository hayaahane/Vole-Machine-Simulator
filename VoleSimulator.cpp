#include "VoleSimulator.h"
#include <iostream>
#include <string>
using namespace std;

VoleSimulator::VoleSimulator()
{
    running = true;
}

void VoleSimulator::start()
{
    int choice;

    while (running)
    {
        displayMenu();
        cin >> choice;

        
        if (cin.fail())
        {
            cin.clear();
            cin.ignore(1000, '\n');
            choice = -1;
        }

        switch (choice)
        {
            case 1:
                handleLoadFile();
                break;
            case 2:
                handleManualMemoryEdit();
                break;
            case 3:
                handleManualRegisterEdit();
                break;
            case 4:
                runAll();
                break;
            case 5:
                runStepByStep();
                break;
            case 6:
                displayMachineState();
                break;
            case 0:
                running = false;
                break;
            default:
                cout << "Invalid choice, try again." << endl;
        }
    }

    cout << "Goodbye!" << endl;
}

void VoleSimulator::displayMenu()
{
    cout << endl;
    cout << "===== Vole Machine Simulator =====" << endl;
    cout << "1. Load program from file" << endl;
    cout << "2. Edit memory" << endl;
    cout << "3. Edit register" << endl;
    cout << "4. Run all" << endl;
    cout << "5. Run step by step" << endl;
    cout << "6. Display machine state" << endl;
    cout << "0. Exit" << endl;
    cout << "Choice: ";
}

void VoleSimulator::handleLoadFile()
{
    string filename;
    int startAddress;

    cout << "Enter file name: ";
    cin >> filename;

    cout << "Enter start address in hex (even number, example 00): ";
    cin >> hex >> startAddress;
    while (cin.fail() || startAddress < 0 || startAddress > 254 || startAddress % 2 != 0)
    {
        cin.clear();
        cin.ignore(1000, '\n');
        cout << "Wrong address, enter it again: ";
        cin >> hex >> startAddress;
    }
    cin >> dec;

    memory.clear();
    cpu.reset();
    cpu.setPC(startAddress);

    
    while (!memory.loadFromFile(filename, startAddress))
    {
        cout << "Error: could not load the file, enter the file name again: ";
        cin >> filename;
    }

    cout << "Program loaded successfully." << endl;
}

void VoleSimulator::handleManualMemoryEdit()
{
    int address, value;

    cout << "Enter address in hex (00 to FF): ";
    cin >> hex >> address;
    while (cin.fail() || address < 0 || address > 255)
    {
        cin.clear();
        cin.ignore(1000, '\n');
        cout << "Wrong address, enter it again: ";
        cin >> hex >> address;
    }

    cout << "Enter value in hex (00 to FF): ";
    cin >> hex >> value;
    while (cin.fail() || value < 0 || value > 255)
    {
        cin.clear();
        cin.ignore(1000, '\n');
        cout << "Wrong value, enter it again: ";
        cin >> hex >> value;
    }
    cin >> dec;

    memory.write(address, value);
    cout << "Memory updated." << endl;
}

void VoleSimulator::handleManualRegisterEdit()
{
    int reg, value;

    cout << "Enter register number in hex (0 to F): ";
    cin >> hex >> reg;
    while (cin.fail() || reg < 0 || reg > 15)
    {
        cin.clear();
        cin.ignore(1000, '\n');
        cout << "Wrong register, enter it again: ";
        cin >> hex >> reg;
    }

    cout << "Enter value in hex (00 to FF): ";
    cin >> hex >> value;
    while (cin.fail() || value < 0 || value > 255)
    {
        cin.clear();
        cin.ignore(1000, '\n');
        cout << "Wrong value, enter it again: ";
        cin >> hex >> value;
    }
    cin >> dec;

    cpu.setRegister(reg, value);
    cout << "Register updated." << endl;
}

void VoleSimulator::runAll()
{
    
    while (!cpu.isHalted())
    {
        cpu.step(memory);
    }

    displayMachineState();
}

void VoleSimulator::runStepByStep()
{
    char answer = 'y';

    while (!cpu.isHalted() && answer == 'y')
    {
        cpu.step(memory);
        displayMachineState();

        if (!cpu.isHalted())
        {
            cout << "Next step? (y/n): ";
            cin >> answer;
        }
    }
}

void VoleSimulator::displayMachineState() const
{
    cout << "CPU:" << endl;
    cpu.printState();

    cout << "Memory:" << endl;
    memory.printMatrix();

    cout << "Screen:" << endl;
    cout << memory.getScreenOutput() << endl;
}