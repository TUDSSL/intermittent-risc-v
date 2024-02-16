// #include <stdio.h>
// #include <stdlib.h>

// extern void rc_test();
// extern void tohost_exit(uintptr_t code);

// static int cntr = 0;

// int scanf(const char *fmt, ...){
//   printf("scanf: %s\n", fmt);
//   return fmt[0];
// }

// int main()
// {
//   char str[10] = {0};
//   scanf("%s", str);

//   for (char *strptr = str; *strptr != '\0'; ++strptr) {
//     cntr++;
//     printf("Hello World!\n");
//   }

//   rc_test();
//   tohost_exit(5);
//   return 0;
// }

int main()
{
  int a = 0xABC;
  int b = 0xDEF;

  a = b;

  return a;
}
