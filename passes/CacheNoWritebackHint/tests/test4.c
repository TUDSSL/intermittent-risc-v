volatile int glob_a = 4;
volatile int glob_b;

int main(void) {
    int a = glob_a; // Read

    if (glob_b) {
      glob_a = 10; // Write
    } else {
      glob_a = 5; // Write
    }

    return 42;
}
