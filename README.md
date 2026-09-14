# terminalBasedPokemon
The final product of a series of 9 iterative assignments in my COM S 3270 class, as I learned C/C++.

## Build and Run

### Requirements

This project requires:

* `g++`
* `make`
* `ncursesw`
* Git

### 1. Clone the repository

```bash
git clone https://github.com/jewing28/terminalBasedPokemon.git
```

Move into the project directory:

```bash
cd terminalBasedPokemon
```

### 2. Install dependencies

#### Ubuntu / Debian / WSL

```bash
sudo apt update
sudo apt install g++ make libncursesw5-dev
```

If `libncursesw5-dev` is unavailable, use:

```bash
sudo apt install libncurses-dev
```

### 3. Compile the project

From the project directory, run:

```bash
make
```

This compiles the source files and creates an executable named `main`.

### 4. Run the game

```bash
./main
```

### 5. Clean build files

To remove the executable and compiled object files:

```bash
make clean
```

To rebuild the project from scratch:

```bash
make clean
make
```

