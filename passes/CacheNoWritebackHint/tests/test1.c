volatile int glob_a = 4;

int main(void) {
    int a = glob_a; // Read
    
    a++;

    glob_a = a; // Write

    return 42;
}
