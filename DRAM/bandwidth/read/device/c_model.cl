int read(__global const int *X, int N) {
  int y = 0;
  for (int i = 0; i < N; i++) {
    if (i%16 == 0) y += X[i];
  }
  return y;
}
