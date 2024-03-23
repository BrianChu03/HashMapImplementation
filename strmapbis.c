/* File: strmapbis.c
 * Course: CS270
 * Project: 3
 * Purpose: Implement functions outlined in strmap.h
 * Author: Brian Chu
 */

#include "strmapbis.h"

/* Creation of the hash function */
int hash_gen(char *key, int numbuckets) {
	// Initialize prime number has value
	unsigned long hash = 10631;
	// Initialize the constant to multiply each character by
	const int FIXED_ODD = 7;

	for(int i = 0; key[i] != '\0'; i++) {
		hash = (FIXED_ODD * hash) + key[i];
	}

	// Modulo to make the index within the size of the bucket array
	hash = hash % numbuckets;
    
    return hash;
}

/* Create a new hashtab, initialized to empty. */
strmap_t *strmap_create(int numbuckets) {
	if (numbuckets > MAX_BUCKETS) { numbuckets = MAX_BUCKETS; }
	if (numbuckets < MIN_BUCKETS) { numbuckets = MIN_BUCKETS; }
	
	// Allocates memory to a new hash map
	strmap_t *temp = (strmap_t *) malloc(sizeof(strmap_t));
	// Dynamically allocates the size of the bucket array
	temp->strmap_buckets = (smel_t**)calloc(numbuckets,sizeof(smel_t));
	
	// Set number of elements to 0
	temp->strmap_size = 0;
	// Set variable of the size ofthe array to track number of buckets
	temp->strmap_nbuckets = numbuckets;

	return temp;
}

void strmap_resize(strmap_t *map, double targetLF) {
	
	double minLF = (1 - LFSLOP) * targetLF;
    double maxLF = (1 + LFSLOP) * targetLF;

	// The number of buckets for the new array
	int adj_numbucket = 0;
	// If the load factor is in the range, return
	if (minLF <= strmap_getloadfactor(map) && strmap_getloadfactor(map) <= maxLF) {
		return;
	}
	// Else, resize the hash table
	else {
		// Calculate the new number of buckets
		adj_numbucket = (int) (strmap_getsize(map) / targetLF);

		// Clip the number of buckets to the maximum and minimum
		if (adj_numbucket > MAX_BUCKETS) { adj_numbucket = MAX_BUCKETS; }
		else if (adj_numbucket < MIN_BUCKETS) { adj_numbucket = MIN_BUCKETS; }

		// Create new bucket array with the adjusted number of buckets and point strmap to new bucket array
		smel_t **new_buckets = (smel_t**)calloc(adj_numbucket,sizeof(smel_t *));

		// Iterate through the old bucket array and grab all values
		for(int i = 0; i < map->strmap_nbuckets; i++) {
		// If the bucket is not empty, iterate through the linked list and take all values
			smel_t *current = map->strmap_buckets[i];

			while (current != NULL) {
				smel_t *next = current->sme_next;
				unsigned long new_hash = hash_gen(current->sme_key, adj_numbucket);
				current->sme_next = new_buckets[new_hash];
				new_buckets[new_hash] = current;
				current = next;
			}
		}

		// Free the old bucket array and set the new bucket array
		free(map->strmap_buckets);

		map->strmap_buckets = new_buckets;
		map->strmap_nbuckets = adj_numbucket;
	}
}

/* Insert an element with the given key and value.
 *  Return the previous value associated with that key, or null if none.
 */
void *strmap_put(strmap_t *m, char *key, void *value) {
	
	// Create the hash from the key
	unsigned long hash = hash_gen(key, m->strmap_nbuckets);

	// Allocate memory for a new element and copy the key and value into it
	smel_t *new_element = (smel_t *)malloc(sizeof(smel_t));
	new_element->sme_key = strdup(key);
	new_element->sme_value = value;

	// If the bucket is not empty
	if (m->strmap_buckets[hash] != NULL) {
		smel_t *current = m->strmap_buckets[hash]; //Holds the current node used to iterate
		smel_t *previous = NULL; //Holds the previous node

		// Have to separately iterate through the list to check for duplicates
		// (otherwise, the new element will be added to the beginning of the list if it is bigger or smaller than the first/second element)
		while (current != NULL) {
			//Ensure there is no duplicate key, if there is, overwrite the value
			if (new_element->sme_key != NULL) { // Avoid seg fault from NULL pointer access
				if (strcmp(new_element->sme_key, current->sme_key) == 0) {
				void *old_value = current->sme_value;
				current->sme_value = new_element->sme_value;
				free(new_element->sme_key);
				free(new_element);
				return old_value;
				}
			}
			previous = current;
			current = current->sme_next;
		}
		
		// Reset iterators
		current = m->strmap_buckets[hash];
		previous = NULL; 
		// If a hash collision occurs (where one key has the same index as another),
		// the new element will be added to the linked list in lexicographic order
		while (current != NULL) {
			// If the key is less than the current node's key, insert the new element before it
			if (strcmp(new_element->sme_key, current->sme_key) < 0 || current->sme_next == NULL) {
				if (previous == NULL && strcmp(new_element->sme_key, current->sme_key) < 0) {   
					//If previous is NULL the new element is now the first element in the list
					new_element->sme_next = current;
					m->strmap_buckets[hash] = new_element;
				}
				else if (current->sme_next != NULL && previous != NULL) {
					//Insert the new element between previous and current
					new_element->sme_next = current;
					current = new_element;
					previous->sme_next = current;
				}
				else {		
					//If current is the last element in the list, insert the new element after it
					new_element->sme_next = NULL;
					previous = current;
					current = new_element;
					previous->sme_next = current;
				}
				m->strmap_size++;
				return NULL;
			}
			// Iterate to the next node
			previous = current;
			current = current->sme_next;
		}
	}

	// If the bucket is empty, set the new element as the first element in the list
	m->strmap_buckets[hash] = new_element;
	// This is to prevent memory leaks
	m->strmap_buckets[hash]->sme_next = NULL;
	m->strmap_size++;
	return NULL;
}
	
/* return the value associated with the given key, or null if none */
void *strmap_get(strmap_t *m, char *key) {
	//Every key has a unique index based on the hash. Redo the hash to find the index.
	unsigned long hash = hash_gen(key, m->strmap_nbuckets);
	
	// If the bucket is not empty, iterate through the linked list to find the key
	if (m->strmap_buckets[hash] != NULL) {
		smel_t *current = m->strmap_buckets[hash];

		while (current != NULL) {
			// If the key is found, return the value
			if (strcmp(current->sme_key, key) == 0) {
				return(current->sme_value);
			}
			// If the key is not found, continue iterating
			current = current->sme_next;
		}
	}
	return NULL;
}

/* remove the element with the given key and return its value.
   Return null if the hashtab contains no element with the given key */
void *strmap_remove(strmap_t *m, char *key) {
	//Every key has a unique index based on the hash. Redo the hash to find the index.
	unsigned long hash = hash_gen(key, m->strmap_nbuckets);

	if (m->strmap_buckets[hash] != NULL) {
		smel_t *current = m->strmap_buckets[hash];
		smel_t *previous = NULL;

		while (current != NULL) {
			// If the key is found, remove the element and return the value
			if (strcmp(current->sme_key, key) == 0) {
				void *old_value = current->sme_value;
				// If the element is the first element in the list, set the bucket to the next element
				if (previous == NULL) {
					m->strmap_buckets[hash] = current->sme_next;
				}
				// If the element is not the first element in the list, set the previous element's next to the next element
				else {
					previous->sme_next = current->sme_next;
				}
				free(current);
				m->strmap_size--;
				return old_value;
			}

			// If the key is not found, continue iterating
			previous = current;
			current = current->sme_next;
		}
	}

	return NULL;
}

/* return the # of elements in the hashtab */
int strmap_getsize(strmap_t *m) {
	return(m->strmap_size);
}

/* return the # of buckets in the hashtab */
int strmap_getnbuckets(strmap_t *m) {
	return(m->strmap_nbuckets);
}

/* print out the contents of each bucket */
void strmap_dump(strmap_t *m) {
	printf("Total Elements = %d.\n", strmap_getsize(m));

	// Iterate through the bucket array
	for(int i = 0; i < m->strmap_nbuckets; i++) {
		smel_t *current = m->strmap_buckets[i];
		current = m->strmap_buckets[i];

		// If the bucket is not empty, iterate through the linked list and print all elements
		if (m->strmap_buckets[i] != NULL) {
			printf("Bucket %d:\n", i);
			
			while (current != NULL) {
				printf("    %s->%p\n", current->sme_key, current->sme_value);
				current = current->sme_next;
			}
		}
	}
}

/* return the current load factor of the map  */
double strmap_getloadfactor(strmap_t *map) {
	return (double)strmap_getsize(map) / (double)strmap_getnbuckets(map);
}

