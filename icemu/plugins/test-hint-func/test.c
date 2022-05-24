
void __cache_hint(char *address_arg) {
    (void)address_arg;
}

void quicksort(int *numbers, int first, int last) {
  int i, j, pivot, temp;

  if (first < last) {
    pivot = first;
    i = first;
    j = last;

    while (i < j) {
      while (numbers[i] <= numbers[pivot] && i < last) i++;
      while (numbers[j] > numbers[pivot]) j--;
      if (i < j) {
        temp = numbers[i];
        numbers[i] = numbers[j];
        numbers[j] = temp;
      }
    }

    temp = numbers[pivot];
    numbers[pivot] = numbers[j];
    numbers[j] = temp;
    quicksort(numbers, first, j - 1);
    quicksort(numbers, j + 1, last);
  }
}

int main() {
  static int numbers[] = {9,2,3,5,6,8,10,1,4,7};

  quicksort(numbers, 0, sizeof(numbers)/sizeof(numbers[0])-1);

  return numbers[1];
}
