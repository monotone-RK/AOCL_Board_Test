int write(__global int *Y, int N) {
  for (int i = 0; i < N; i++) Y[i] = i;
  return (int)11;
}
