#include"Memory.h"
#include<fstream>
#include <sstream>
#include <cctype>
#include <cstddef>
#include <vector>
#include <iostream>
#include <iomanip>
using namespace std;


Memory::Memory()
{
   clear();
}

void Memory::clear()
{
    for(int i=0 ; i<256 ; i++)
    {
        data[i]=0;
    }
    screenBuffer="";

}


uint8_t Memory::read(uint8_t address) const
{
    return data[address];
}


void Memory::write(uint8_t address , uint8_t value)
{
    if (address==0xFF)
    {
        screenBuffer+= static_cast<char>(value);
    }
    else
    {
        data[address]=value;
    }

}



bool Memory::isEvenAddress(uint8_t address) const
{
    return address % 2 == 0;
}



bool Memory::loadFromFile(const std::string & filename , uint8_t startAddress)
{
    if (!isEvenAddress(startAddress))
    {
        return false;
    }

    ifstream file(filename);
    if (!file.is_open())
    {
        return false;
    }

    string line ;
    vector<uint8_t> program;
     while (getline(file , line))
     {
        stringstream ss(line);
        string code;
        ss >> code;

        if(code.empty() || code[0]== ';')
        {
            continue;
        }

        string extra;

        if (ss >> extra)
        {
           return false;
        }

        if(code.length() !=4 )
        {
            return false;
        }

        for(int i=0 ; i<4 ; i++)
        {
            if (!isxdigit(static_cast<unsigned char>(code[i])))
            {
                return false;
            }
        }

        int opcode = stoi(code.substr(0,1) , nullptr , 16);
        if (opcode<1 || opcode>12)
        {
            return false;
        }
        
        program.push_back(stoi(code.substr(0,2) , nullptr , 16));
        program.push_back(stoi(code.substr(2,2) , nullptr , 16));
     }

     if (startAddress + program.size() > 0xFE)
     {
        return false;
     }

     for( size_t i=0 ; i < program.size() ; i++ )
     {
        write(startAddress +i , program[i]);
     }

     return true;
     
    
}


string Memory::getScreenOutput() const
{
    return screenBuffer;
}


void Memory::printMatrix() const
{
    cout << "--- Memory Matrix (00 to FF) ---\n";

    cout << "     ";
    cout << uppercase << hex << setfill('0');

    for (int j = 0; j < 16; j++)
    {
        cout << setw(2) << j << " ";
    }

    cout << endl;
    cout << "     ________________________________________________" << endl;


    for (int i = 0; i < 16; i++)
    {
        int rowAddress = i * 16;

        cout << setw(2) << rowAddress << " | ";

        for (int j = 0; j < 16; j++)
        {
            int address = rowAddress + j;

            cout << setw(2) << (int)data[address] << " ";
        }

        cout << endl;
    }

    cout << dec << setfill(' ');
}
