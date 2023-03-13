#include "parallel.h"
#include "random.h"

// For timing parts of your code.
#include "get_time.h"

// For computing sqrt(n)
#include <math.h>

using namespace parlay;

// Some helpful utilities
namespace {

// returns the log base 2 rounded up (works on ints or longs or unsigned
// versions)
template <class T>
size_t log2_up(T i) {
  assert(i > 0);
  size_t a = 0;
  T b = i - 1;
  while (b > 0) {
    b = b >> 1;
    a++;
  }
  return a;
}

}  // namespace


struct ListNode {
  ListNode* next;
  size_t rank;
  ListNode(ListNode* next) : next(next), rank(std::numeric_limits<size_t>::max()) {}
};

// Serial List Ranking. The rank of a node is its distance from the
// tail of the list. The tail is the node with `next` field nullptr.
//
// The work/depth bounds are:
// Work = O(n)
// Depth = O(n)
void SerialListRanking(ListNode* head) {
  size_t ctr = 0;
  ListNode* save = head;
  while (head != nullptr) {
    head = head->next;
    ++ctr;
  }
  head = save;
  --ctr;  // last node is distance 0
  while (head != nullptr) {
    head->rank = ctr;
    head = head->next;
    --ctr;
  }
  //   ListNode* temp = save;
  // while (temp != nullptr) {
  //   std::cout << temp->rank << " ; " << std::endl;
  //   temp = temp->next;
  // }
}

// Wyllie's List Ranking. Based on pointer-jumping.
//
// The work/depth bounds of your implementation should be:
// Work = O(n*\log(n))
// Depth = O(\log^2(n))
void WyllieListRanking(ListNode* L, size_t n) {
  size_t* D = (size_t*)malloc(n * sizeof(size_t));
  size_t* succ = (size_t*)malloc(n * sizeof(size_t));
  size_t* succbackup = (size_t*)malloc(n * sizeof(size_t));
  
  parallel_for(0, n, [&](size_t i) { 
    if (L[i].next == nullptr) {
      D[i] = 0;
      succ[i] = i;
    } else {
      D[i] = 1;
      parallel_for(0, n, [&](size_t j) {
        if (L[i].next == &L[j]) {
          succ[i] = j;
        }
      });
    }
    succbackup[i] = succ[i]; 
  });
  

  size_t hopbound = log2_up(n);
  //for (size_t i = 0; i < n; i++) {
  //  if (L[i].next == nullptr) {
  //    D[i] = 0;
  //    succ[i] = i;
  //  } else {
  //    D[i] = 1;
  //    for (size_t j = 0; j < n; j++){
  //	if (L[j].next == &L[j]) {
  //	  succ[i] = j;
  //	}
  //  }
  //}
  //succbackup[i] = succ[i];
  //}
  for (size_t i = 0; i < hopbound; i++) {
    parallel_for(0, n, [&](size_t i) {
      if (succ[i]) {
        D[i] = D[i] + D[succ[i]];
      }
      succbackup[i] = succ[succ[i]];
      succ[i] = succbackup[i];
    });
  }

  parallel_for(0, n, [&](size_t i) { L[i].rank = D[i]; });

  // ListNode* temp = L;
  // while (temp != nullptr) {
  //   std::cout << temp->rank << " ; " << temp <<std::endl;
  //   temp = temp->next;
  // }
  free(D);
  free(succ);
  free(succbackup);
}


// Sampling-Based List Ranking
//
// The work/depth bounds of your implementation should be:
// Work = O(n) whp
// Depth = O(\sqrt(n)* \log(n)) whp
void SamplingBasedListRanking(ListNode* L, size_t n, long num_samples=-1, parlay::random r=parlay::random(0)) {
  // Perhaps use a serial base case for small enough inputs?

  if (num_samples == -1) {
    num_samples = sqrt(n);
  }

}

