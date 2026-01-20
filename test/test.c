int main(void) {
  int a = 5;
  int b = 3;
  int c;

  c = (a * b) + (a - b);

  if (a < b) {
    c = c + 10;
  } else {
    c = c - 2;
  }

  int d = (c > 10) ? (c + 1) : (c - 1);

  if (d != 0)
    return d;

  return 0;
}
