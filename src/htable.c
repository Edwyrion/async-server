// ==============================================================================
//                             Generic Hash table
// ==============================================================================
//
// Description: This library provides basic implementation of a generic hash table
// using a linked list for collision resolution. To use this library, user must
// provide a hash function and a comparison function for the keys and optionally
// a wrapper function for the retrieval of the hash value for type safety.
//
// MIT License
//
// Copyright (c) 2024 Erik A. Rapp
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// ==============================================================================

#include "htable.h"

// --- Static Function Definitions --- //

/// @brief Retrieve the hash value for the key.
/// @param table The hash table to retrieve the hash value from.
/// @param key The key for the hash node.
/// @return The hash value for the key.
static size_t get_hash (const htable_t *table, const void *key) {
    return table->hash(key) % table->size;
}

static void *htable_default_copy (const void *src) {
    return (void *) src;
}

static void htable_default_free (void *src) {
    (void) src;
    return;
}

// --- Function Definitions --- //

/// @brief Create a hash table with the specified size.
htable_t *htable_create (size_t size, htable_hash_t hash, htable_keq_t keq, const struct callbacks *cbs) {

    if (size == 0 || hash == NULL || keq == NULL) {
        return NULL;
    }

    // Allocate memory for the hash table.
    htable_t *table = NULL;

    if ((table = calloc(1U, sizeof(*table))) == NULL) {
        return NULL;
    }

    // Allocate memory for the hash table.;
    if ((table->table = calloc(size, sizeof(*table->table))) == NULL) {
        free(table);
        return NULL;
    }

    // Initialize the remaining fields of the hash table.
    table->size = size;

    // Callbacks.
    table->hash = hash;
    table->keq = keq;

    table->cbs.kcpy = htable_default_copy;
    table->cbs.vcpy = htable_default_copy;
    table->cbs.kfree = htable_default_free;
    table->cbs.vfree = htable_default_free;
    
    // Update the callbacks if provided.
    if (cbs != NULL) {
        table->cbs.kcpy = cbs->kcpy ? cbs->kcpy : table->cbs.kcpy;
        table->cbs.vcpy = cbs->vcpy ? cbs->vcpy : table->cbs.vcpy;
        table->cbs.kfree = cbs->kfree ? cbs->kfree : table->cbs.kfree;
        table->cbs.vfree = cbs->vfree ? cbs->vfree : table->cbs.vfree;
    }

    return table;
}

/// @brief Destroy the hash table and free resources.
void htable_destroy (htable_t *table) {

    if (table == NULL || table->table == NULL) {
        return;
    }

    // Free the hash nodes.
    for (size_t idx = 0; idx < table->size; idx++) {

        struct htable_node *current = table->table[idx];

        // Traverse the linked list.
        do {
            struct htable_node *next = current->next;

            // Free the key and value.
            table->cbs.kfree(current->key);
            table->cbs.vfree(current->value);

            // Free the hash node.
            free(current);

            current = next;
        } while (current != NULL);
    }

    // Free the hash table array.
    free(table->table);

    // Free the hash table.
    free(table);
}

/// @brief Insert a key-value pair into the hash table.
int htable_insert (htable_t *table, const void *key, const void *value) {

    if (table == NULL || table->table == NULL || value == NULL) {
        return -1;
    }

    // Calculate the hash index.
    const size_t hashed_key = get_hash(table, key);

    // Get the current hash node corresponding to the hashed key.
    struct htable_node *current = table->table[hashed_key];

    // Check if there is another hash node present already, if not, create a new one.
    if (current == NULL) {
        struct htable_node *new_node = NULL;

        if ((new_node = malloc(sizeof(*new_node))) == NULL) {
            return -2;
        }

        new_node->key = table->cbs.kcpy(key);
        new_node->value = table->cbs.vcpy(value);
        new_node->next = NULL;

        table->table[hashed_key] = new_node;
    }
    // Traverse the linked list.
    else {
        struct htable_node *prev = NULL;
        // Check if the key already exists in the current linked list.
        do {
            if (table->keq(current->key, key)) {
                // Free the previous value and update it with the new value.
                table->cbs.vfree(current->value);
                current->value = table->cbs.vcpy(value);
                return 0;
            }
            // Move to the next hash node.
            prev = current;
            current = current->next;
        } while (current != NULL);

        // If the key does not exist in the linked list, create a new hash node.
        struct htable_node *new_node = NULL;

        if ((new_node = malloc(sizeof(*new_node))) == NULL) {
            return -2;
        }

        new_node->key = table->cbs.kcpy(key);
        new_node->value = table->cbs.vcpy(value);
        prev->next = new_node;
    }

    return 0;
}

/// @brief Remove a key-value pair from the hash table.
int htable_remove (htable_t *table, const void *key) {

    if (table == NULL || table->table == NULL) {
        return -1;
    }

    // Calculate the hash index.
    const size_t hashed_key = get_hash(table, key);

    struct htable_node *current = table->table[hashed_key];
    struct htable_node *prev = NULL;

    while (current != NULL) {
        // Check if the key exists in the linked list and remove it.
        if (table->keq(current->key, key)) {
            
            // Link the previous hash node to the next hash node.
            if (prev == NULL) {
                table->table[hashed_key] = current->next;
            } else {
                prev->next = current->next;
            }

            // Free the key and value.
            table->cbs.kfree(current->key);
            table->cbs.vfree(current->value);

            // Free the hash node.
            free(current);

            return 0;
        }
        
        // Move to the next hash node.
        prev = current;
        current = current->next;
    }

    return -1;
}

/// @brief Get the hash node for the specified key.
void *htable_get (htable_t *table, const void *key) {

    if (table == NULL || table->table == NULL) {
        return NULL;
    }

    // Calculate the hash index.
    const size_t hashed_key = get_hash(table, key);

    struct htable_node *current = table->table[hashed_key];

    // Traverse the linked list to find the hash node.
    while (current != NULL) {
        // Check if the key matches the current hash node.
        if (table->keq(current->key, key)) {
            return current->value;
        }

        // Move to the next hash node.
        current = current->next;
    }

    return NULL;
}