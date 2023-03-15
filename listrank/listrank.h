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
  ListNode* samplesucc;
  bool sampleflag;
  ListNode(ListNode* next) : next(next), rank(std::numeric_limits<size_t>::max()), samplesucc(nullptr), sampleflag(false) {}
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
  size_t* Dbackup = (size_t*)malloc(n * sizeof(size_t));
  size_t* succ = (size_t*)malloc(n * sizeof(size_t));
  size_t* succbackup = (size_t*)malloc(n * sizeof(size_t));
  
  parallel_for(0, n, [&](size_t i) { 
    if (L[i].next == nullptr) {
      D[i] = 0;
      succ[i] = i;
    } else {
      D[i] = 1;

      // following is a pointer calculation, maynot be correct
      succ[i] = i + (size_t)(L[i].next-&L[i]); // / sizeof(ListNode);
    }
    succbackup[i] = succ[i]; 
    Dbackup[i] = D[i];
  });

  size_t hopbound = log2_up(n);

  for (size_t j = 0; j < hopbound; j++) {
    parallel_for(0, n, [&](size_t i) {
      if (succ[i] != i) {
        Dbackup[i] = D[i] + D[succ[i]];
        succbackup[i] = succ[succ[i]];
      }
    });
    parallel_for(0, n, [&](size_t i) { 
      D[i] = Dbackup[i];
      succ[i] = succbackup[i]; 
    });
  }


  parallel_for(0, n, [&](size_t i) { L[i].rank = D[i]; });

  // std::cout << D[0] << " " << L[0].rank << std::endl;

  // ListNode* temp = L;
  // while (temp != nullptr) {
  //   std::cout << "(" << temp->rank << " , " << temp << ") ; ";
  //   temp = temp->next;
  // }
  // std::cout << "END OF OUTPUT" << std::endl;
  free(D);
  free(Dbackup);
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

  size_t* sampling = (size_t*)malloc(n * sizeof(size_t));
  size_t* weight = (size_t*)malloc(n * sizeof(size_t));
  size_t lastindex = 0;

  parallel_for(0, n, [&] (size_t i) {
    sampling[i] = r[i] % num_samples;
  });
  parallel_for(0, n, [&] (size_t i) {
    if (L[i].next == nullptr) {
      lastindex = i;
    }
    L[i].rank = n;
    L[i].sampleflag = false;
  });
  L[lastindex].samplesucc = nullptr;
  L[lastindex].rank = 0;
  size_t identifier = sampling[0];
  
  parallel_for(0, n, [&] (size_t i) {
    if (i == lastindex) {
      // this is the special case
      weight[i] = 0;
    } else {
      if (sampling[i] == identifier) {
        size_t w = 0;
        ListNode* t = &L[i];
        while (t->next != nullptr) {
          t = t->next;
          size_t index = (size_t) (t-L);
          if (index == lastindex) {
            break;
          }
          if (sampling[index] == identifier) {
            break;
          } else {
            w = w + 1;
          }
        }
        weight[i] = w;
        L[i].rank = w;
        L[i].samplesucc = t;
        L[i].sampleflag = true;
      } else {
        weight[i] = n+1;
      }
    }
  });

  // debugging message to see what the list looks like before
  // assigning rank to non sample nodes

  // ListNode* temp = L;
  // while (temp != nullptr) {
  //   size_t index = (size_t) (temp - L);
  //   std::cout << "(" << index << "," << temp->rank << ")->";
  //   temp = temp->next;
  // }
  // std::cout << std::endl;
  
  // sample node rank calculation
  size_t ctr = 0;
  ListNode* head = L;
  while (head != nullptr) {
    ctr = ctr + head->rank + 1;
    head = head->samplesucc;
  }
  head = L;
  --ctr;  // last node is distance 0
  while (head != nullptr) {
    size_t temp = head->rank;
    head->rank = ctr;
    head = head->samplesucc;
    ctr = ctr - temp - 1;
  }

  // non sample node rank calculation
  parallel_for(0, n, [&](size_t i) {
    // ListNode curr = L[i];
    if (L[i].sampleflag) {
      // this is a sample node
      // ListNode* hd = &curr;
      ListNode* hd = &(L[i]);
      size_t currcounter = hd->rank;
      while (hd != L[i].samplesucc) {
        hd->rank = currcounter;
        currcounter = currcounter - 1;
        hd = hd->next;
      }
    }
  });

  // std::cout << L[0].rank << " " << L[1].rank << std::endl;

  // final check and output message
  // std::cout << "FINAL CHECK" << std::endl;
  // temp = L;
  // while (temp != nullptr) {
  //   size_t index = (size_t) (temp - L);
  //   std::cout << "(" << index << "," << temp->rank << ")->";
  //   temp = temp->next;
  // }
  // std::cout << std::endl;
  // temp = L;
  // while (temp != nullptr) {
  //   size_t index = (size_t) (temp - L);
  //   std::cout << "(" << index << " , " << sampling[index] << " , " << weight[index] << ") ; ";
  //   temp = temp->next;
  // }
  // std::cout << std::endl;
  // temp = L;
  // while (temp != nullptr) {
  //   size_t index = (size_t) (temp - L);
  //   size_t nindex = (size_t) (temp->samplesucc - L);
  //   std::cout << "(" << index << " -> " << nindex << ") -> ";
  //   temp = temp->next;
  // }
  // std::cout << std::endl;
  // std::cout << "END OF OUTPUT" << std::endl;

  free(sampling);
  free(weight);

}

