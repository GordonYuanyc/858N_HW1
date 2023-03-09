#include "parallel.h"

using namespace parlay;

// A serial implementation for checking correctness.
//
// Work = O(n)
// Depth = O(n)
template <class T, class F>
T scan_inplace_serial(T *A, size_t n, const F& f, T id) {
  T cur = id;
  for (size_t i=0; i<n; ++i) {
    T next = f(cur, A[i]);
    A[i] = cur;
    cur = next;
  }
  return cur;
}

template <class T, class F>
T scan_up(T *A, T *L, size_t n, const F& f) {
  if (n == 1) {
    return A[0];
  } else {
    size_t m = n / 2;
    T v1, v2;
    auto f1 = [&]() { v1 = scan_up(A, L, m, f); };
    auto f2 = [&]() { v2 = scan_up(A+m, L+m, n-m, f); };
    par_do(f1, f2);
    L[m-1] = v1;
    // std::cout << "v1: " << v1 << " " << "v2: " << v2 << std::endl;
    // std::cout << "f: " << f(v1, v2) << " m " << m  << " L " << L[0] << " " << L[1] << " " << L[2] << " " << L[3] << " " << L[4] << " " << L[5] << std::endl;
    return f(v1, v2);
  }
}

// doesn't seems to be necessary but should generate the correct R with sum
template <class T, class F>
void scan_down(T *R, T *L, size_t n, const F& f, T s) {
  if (n == 1) {
    R[0] = s;
    return;
  } else {
    size_t m = n / 2;
    auto f1 = [&] () { scan_down(R, L, m, f, s); };
    auto f2 = [&] () { scan_down(R+m,L+m, n-m, f, f(s, L[m-1])); };
    par_do(f1, f2);
    return;
  }
}

// Parallel in-place prefix sums. Your implementation can allocate and
// use an extra n*sizeof(T) bytes of memory.
//
// The work/depth bounds of your implementation should be:
// Work = O(n)
// Depth = O(\log(n))
template <class T, class F>
T scan_inplace(T *A, size_t n, const F& f, T id) {
  T* L = (T*)malloc((n-1) * sizeof(T));
  T* R = (T*)malloc(n * sizeof(T));
  // std::cout << "A: ";
  // for (int i = 0; i < 6; i++) {
  //   std::cout << A[i] << ";";
  // }
  // std::cout << " end of A" << std::endl;
  T total = scan_up(A, L, n, f);
  scan_down(R, L, n, f, id);
  // std::cout << "L: ";
  // for (int i = 0; i < 5; i++) {
  //   std::cout << L[i] << ";";
  // }
  // std::cout << " end of L" << std::endl;
  // std::cout << "id: " << id << std::endl;
  // std::cout << "R: ";
  // for (int i = 0; i < 6; i++) {
  //   std::cout << R[i] << ";";
  // }
  // std::cout << " end of R" << std::endl;
  return total;  // TODO
}
