volatile int glob_a = 4;

int main(void) {
    int a = glob_a;
    int b = glob_a;
    int c = glob_a;
    
    a++;

    glob_a = a;
    glob_a = 1;
    glob_a = 2;

    return 42;
}
