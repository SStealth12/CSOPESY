// Group 13: Miguel Estanol, Aaron Nicolas
// Section: S19

#include <ctime>
#include <cstdlib>
#include "globals.h"
#include "osloop.h"

int main() {
    // Seed the random number generator for the entire program
    std::srand(static_cast<unsigned int>(std::time(0)));

    // Start the main OS loop
    OSLoop();

    return 0;
}