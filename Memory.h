#ifndef MEMORY_H
#define MEMORY_H

#include<cstdint>
#include<string>


class Memory
{
    private:
    uint8_t data[256];
    std::string screenBuffer;

    public:
    Memory();
    void clear();
    uint8_t read(uint8_t address) const;
    void write(uint8_t address , uint8_t value);
    bool loadFromFile(const std::string & filename , uint8_t startAddress=0);
    bool isEvenAddress(uint8_t address) const;
    std::string getScreenOutput() const;
    void printMatrix() const;

};
#endif