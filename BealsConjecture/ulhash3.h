#ifndef INT_HASH_H
#define INT_HASH_H

/* Hash class designed specifically for mapping unsigned int -> {0, 1} */
typedef struct {
	int nBuckets;
	unsigned int** buckets;
} ulhash;

// Initialize a new ulhash
ulhash* ulhash_create(unsigned int);

// Insert or overwrite an existing entry in the hash
int ulhash_set(ulhash*, unsigned int key);

// Lookup
unsigned int ulhash_find(ulhash*, unsigned int key);

// Total keys in the hash
unsigned int ulhash_count(ulhash*);

// If the keyset is fixed, this can boost key lookup speed
void ulhash_opt(ulhash* t);
unsigned int ulhash_opt_find(ulhash*, unsigned int key);

// Clean up
void ulhash_free(ulhash* t);

void ulhash_print_stats(ulhash* t);

#endif
