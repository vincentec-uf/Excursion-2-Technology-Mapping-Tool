# Excursion 2: Technology Mapping Tool

## Overview
This program implements a basic technology mapping tool for Boolean circuits.

Given a Boolean netlist consisting of INPUT, OUTPUT, AND, OR, and NOT operations, the program:
1. Parses the netlist and builds a tree representation.
2. Converts the logic into a NAND-NOT tree.
3. Computes the minimal implementation cost using the given technology library.
4. Outputs the final minimum cost to `output.txt`.

---

## Technology Library

The following gates are supported with their associated costs:

Gate | Cost
Not  :  2
NAND2:  3
AND2 :  4
OR2  :  4
NOR2 :  6
AOI21:  7
AOI22:  7

The program performs structural matching on the NAND-NOT tree to determine the minimum cost implementation.

---

## File Structure

- `parser.cpp`  
  Parses the input netlist and constructs the circuit tree.

- `convertNandNot.cpp`  
  Converts the original Boolean tree into an equivalent NAND-NOT representation.

- `calculateMinCost.cpp`  
  Recursively computes the minimal cost using dynamic programming and pattern matching against the technology library.

- `test_all.cpp`  
  Main driver file that runs the full flow and writes the result to `output.txt`.

- `Node.h`  
  Defines the Node structure used to represent the circuit.

- `netlist.txt`  
  Input file containing the Boolean netlist.

- `output.txt`  
  Output file containing the final minimal cost.

---

## Compilation

Run the following command in the terminal:

```bash
g++ -std=c++11 parser.cpp convertNandNot.cpp calculateMinCost.cpp test_all.cpp -o excursion2

./excursion2
