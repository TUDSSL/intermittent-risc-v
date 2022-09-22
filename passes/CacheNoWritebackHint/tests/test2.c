volatile int glob_a = 4;

int main(void) {
    glob_a = 10; // Write

    int a = glob_a; // Read

    return 42;
}
