int read(__global const int *X, int index, int value) {
  return (X[index] == value) ? 50 : 0; // maybe 50 cycles
}
