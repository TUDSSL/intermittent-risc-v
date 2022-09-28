volatile int glob_a = 4;
volatile int glob_b;

int main(void) {

    do {
      glob_a = 1;   // Write
      int a = glob_a;   // Read

    } while (glob_b);

    return 42;
}
