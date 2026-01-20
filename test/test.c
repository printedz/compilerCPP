int main(void) {
    int a = 6 * 7;
    int b = (a + 1) * (a - 1);
    int c;
    c = (b / a) + (a % 5) + (~0);
    int d = (c == 0) || (a > b);
    int e = (a < b) && (b != 0);
    return d + e + (a / 7);
}
