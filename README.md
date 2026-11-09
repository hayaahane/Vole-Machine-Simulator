# 💻 Vole Machine Simulator

<p align="center">
  **C++ Vole Machine Simulator**
  
  Simulating machine instructions, memory operations, and CPU execution.
</p>

---

## 🚀 About the Project

Vole Machine Simulator is a C++ application designed to simulate the operation of the Vole machine. It allows users to load hexadecimal programs, execute instructions, and monitor the internal machine state.

## ✨ Features

- 📂 Load programs from text files.
- ▶️ Run programs automatically.
- ⏯️ Execute instructions step by step.
- 🧠 Display CPU registers and Instruction Register (IR).
- 💾 View memory contents.
- 🖥️ Display screen output.
- ✅ Validate machine instructions.

## 🏗️ Project Structure

| Class | Responsibility |
|---|---|
| CPU | Fetches, decodes, validates, and executes instructions. |
| Memory | Manages memory and loads programs. |
| ALU | Performs arithmetic and logical operations. |
| VoleSimulator | Controls the simulator and user interaction. |

## ⚙️ Technologies Used

- C++
- Object-Oriented Programming (OOP)
- Separate Compilation
- Git & GitHub

## ▶️ How to Run

**1. Compile the project**

```bash
g++ main.cpp CPU.cpp ALU.cpp Memory.cpp VoleSimulator.cpp -o VoleSimulator.exe
```

**2. Run the application**

```bash
./VoleSimulator.exe
```

## 🧪 Testing

The simulator was tested using multiple Vole machine programs to verify instruction execution and expected results.

## 🖥️ Graphical User Interface (GUI)

The project includes a Graphical User Interface (GUI) that provides a visual and user-friendly way to interact with the Vole Machine Simulator.

### GUI Features
- Load Vole Machine programs.
- Execute programs and control the simulation.
- View CPU registers and the Instruction Register (IR).
- Monitor memory contents.
- Display screen output.
- Interact with the simulator through a graphical interface.

## 👥 Team Members

| Name | Contribution |
|---|---|
| Sara Hamada Mohamed | 20250270 | 20250270@stud.fci-cu.edu.eg  | CPU Class |
| Haneen Osama Reda | 20250206 | 20250206@stud.fci-cu.edu.eg | Memory Class |
| Haya Hany Fathy | 20250725 | 20250725@stud.fci-cu.edu.eg | ALU Class |
| Nermeen Hesham Ezz El Din | 20250706 | 20250706@stud.fci-cu.edu.eg | VoleSimulator Class |

---
<p align="center">
  **Vole Machine Simulator — CS213 OOP Assignment**
</p>
