volatile int glob_a = 4;

int main(void) {
    int a = glob_a;
    int b = glob_a;
    
    a++;
    b++;

    glob_a = a;
    glob_a = b;

    return 42;
}
