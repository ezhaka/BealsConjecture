#include "ulhash3.h"
#include <stdlib.h>
#include <stdio.h>

/*
typedef struct {
	int nBuckets;
	unsigned int** buckets; // first element is count<<16 + #alloced
} ulhash;
*/

//#define HASH(k) (k)

//unsigned int HASH(unsigned int key)
//{idx
#define HASH(idx) \
  idx += (idx << 12); \
  idx ^= (idx >> 22); \
  idx += (idx << 4);  \
  idx ^= (idx >> 9);  \
  idx += (idx << 10); \
  idx ^= (idx >> 2);  \
  idx += (idx << 7);  \
  idx ^= (idx >> 12)

// Initialize a new ulhash
ulhash* ulhash_create(unsigned int size) {
	ulhash* r = (ulhash*)malloc(sizeof(ulhash));
	
	r->nBuckets = size;
	r->buckets = (unsigned int**) calloc( size, sizeof(int*) );
	
	return r;
}

// Insert or overwrite an existing entry in the hash
int ulhash_set(ulhash* t, unsigned int key) {
	unsigned int idx = key;
	HASH(idx);
	idx = idx % t->nBuckets;
	
	unsigned int* bucket = t->buckets[idx];
	if (!bucket) {
		bucket = (unsigned int*)malloc(2 * sizeof(int));
		bucket[0] = (2 << 16) + 0;
		t->buckets[idx] = bucket;
		//printf("Allocated bucket %d at 0x%08x\n", idx, bucket);
	}
	
	// If it's already there, we don't have to insert it
	int cnt = bucket[0] & 0xffff;
	for (int i=1; i<=cnt; i++) {
		if (bucket[i] == key) {
			//printf("Found key %u\n", key);
			return 1;
		}
	}
	
	// Is there room for another key?
	int alloced = bucket[0] >> 16;
	if (alloced <= cnt + 1) {
		// Double the number allocated
		alloced *= 2;
		bucket = (unsigned int*)realloc( bucket, alloced * sizeof(int) );
		t->buckets[idx] = bucket;
		bucket[0] = cnt + (alloced << 16);
		//printf("Reallocated bucked %d, capacity %d at 0x%08x\n", idx, alloced, bucket);
	}
	
	// Add the new key to the end of the list and increment the key count
	cnt++;
	bucket[0] = cnt + (alloced << 16);
	bucket[cnt] = key;
	//printf("set bucket[%d][%d] = %u\n", idx, cnt, key);
	return 0;
}

// Lookup
unsigned int ulhash_find(ulhash* t, unsigned int key) {
	unsigned int idx = key;
	HASH(idx);
	idx = idx % t->nBuckets;
	
	unsigned int* bucket = t->buckets[idx];
	if (!bucket) return 0;
	
	int cnt = bucket[0] & 0xffff;
	for (int i=1; i<=cnt; i++) {
		if (bucket[i] == key) return 1;
	}
	return 0;
}

// Count the number of entries in the hash table
unsigned int ulhash_count(ulhash* t) {
	unsigned int count = 0;
	
	for (int i=0; i<t->nBuckets; i++) {
		if (t->buckets[i])
			count += (t->buckets[i][0] & 0xffff);
	}
	return count;
}

void ulhash_free(ulhash* t) {
  printf("t->nBuckets = %d", t->nBuckets);

	for (int i=0; i<t->nBuckets; i++) {
		if (t->buckets[i]) free(t->buckets[i]);
	}

	free(t);
}



// Sort the contents of each bucket
static int qsortcmp(const void* a, const void* b) {
	unsigned int al, bl;
	
	al = *(unsigned int*)a;
	bl = *(unsigned int*)b;
	
	if (al < bl) return -1;
	if (al == bl) return 0;
	return 1;
}

void ulhash_opt(ulhash* t) {
	for (int i=0; i<t->nBuckets; i++) {
		unsigned int* bucket = t->buckets[i];
		if (bucket) {
			int cnt = bucket[0] & 0xffff;
			qsort(bucket+1, cnt, sizeof(unsigned int), qsortcmp);
		}
	}
}

// Search, taking advantage of the fact that the contents
// of each bucket are in ascending numerical order

unsigned int ulhash_opt_find(ulhash* t, unsigned int x) {
	unsigned int idx = x;
	HASH(idx);
	idx = idx % t->nBuckets;
	
	unsigned int* bucket = t->buckets[idx];
	unsigned int y;

	if (!bucket)
		return 0;
	
	int cnt = bucket[0] & 0xffff;
	
	switch (cnt) {
		case 1:
			return x == bucket[1];
		case 2:
			y = bucket[2];
			if (x==y)
				return 1;
			if (x<y)
				return x == bucket[1];
			return 0;
		case 3:
			y = bucket[2];
			if (x==y)
				return 1;
			if (x<y)
				return x==bucket[1];
			else
				return x==bucket[3];
		case 4:
			y = bucket[2];
			if (x==y) return 1;
			if (x<y)
				return x==bucket[1];
			else {
				y = bucket[3];
				if (x==y) return 1;
				if (x>y)  return x==bucket[4];
				return 0;
			}
			
		default: {
			// Do a full binary search
			int left=1, right=cnt;
		        
		        while (left < right) {
		                int mid = ((right-left)>>1) + left;
		                y = bucket[mid];
		                //printf("l/m/r: %d/%d/%d\n", left, mid, right);
		                //printf("x/y:   %d/%d\n", x, y);
		                if (y==x) return 1;
		                if (x<y)
		                        right = mid-1;
		                else
		                        left = mid+1;
		        }
		        return (left==right && x==bucket[left]);
		}
	}
}

void ulhash_print_stats(ulhash* t) {
	int cnt=0, empty=0, maxCnt=0, alloced;
	int cntSum[10] = {0,0,0,0,0,0,0,0,0,0};
	int i;
	
	alloced = t->nBuckets * sizeof(unsigned int*) + sizeof(ulhash);
	
	for (i=0; i<t->nBuckets; i++) {
		unsigned int* bucket = t->buckets[i];
		if (bucket) {
			int c = bucket[0] & 0xffff;
			int a = bucket[0] >> 16;
			
			if (c < 10) cntSum[c]++;
			
			cnt += c;
			if (c > maxCnt) maxCnt = c;
			
			alloced += a * sizeof(unsigned int);
		} else {
			empty++;
		}
	}
	
	printf("Hash statistics:\n");
	printf("Key count: %d\n", cnt);
	printf("Buckets:   %d\n", t->nBuckets);
	printf("Load:      %f\n", 1.0 * cnt/t->nBuckets);
	printf("Max/buck:  %d\n", maxCnt);
	printf("Empty bs:  %d\n", empty);
	printf("avg ne:    %f\n", 1.0*cnt/(t->nBuckets-empty));
	printf("Mem usage: %d\n", alloced);
	
	printf("Small bucket counts:\n");
	for (i=0;i<10;i++) {
		printf("  %d - %d\n", i, cntSum[i]);
	}
}
