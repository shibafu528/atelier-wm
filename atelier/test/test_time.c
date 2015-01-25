#include <stdio.h>

void PrintCurrentTime(char* output, int length);

int main(int argc, char* argv[]) {
    char buffer[64];
    PrintCurrentTime(buffer, 64);
    printf("%s\n", buffer);
    return 0;
}
