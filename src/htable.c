// ==============================================================================
//                             Generic Hash map
// ==============================================================================
//
// Description: This library provides basic implementation of a generic hash map
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

// --- Function Definitions --- //

/// @brief Create a hash map with the specified size.
htable_t *htable_create (size_t size, hash_func_t hash, comp_func_t comp) {

    assert(size > 0);

    // Allocate memory for the hash map.
    htable_t *map = malloc(sizeof(*map));

    if (map == NULL) {
        LOG_ERROR("Error creating hash map: memory allocation failed");
        return NULL;
    }

    // Allocate memory for the hash table.
    map->table = malloc(size * sizeof(*map->table));

    if (map->table == NULL) {
        LOG_ERROR("Error creating hash table: memory allocation failed");
        free(map);
        return NULL;
    }

    // Zero out the memory for the hash table.
    memset(map->table, 0, size * sizeof(*map->table));

    // Initialize the remaining fields of the hash map.
    map->size = size;

    // Callbacks.
    map->hash = hash;
    map->comp = comp;

    return map;
}

/// @brief Destroy the hash map and free resources.
void htable_destroy (htable_t *map) {

    assert(map);

    if (map == NULL) {
        LOG_ERROR("Error destroying hash map: map is NULL");
        return;
    }

    if (map->table != NULL) {
        free(map->table);
    }

    // Free the hash map.
    free(map);
}

/// @brief Insert a key-value pair into the hash map.
int htable_insert (htable_t *map, const void *key, const void *value) {

    assert(map && map->table && value);

    // Calculate the hash index.
    const size_t hashed_key = get_hash(map, key);

    // Get the current hash node corresponding to the hashed key.
    hnode_t *current = map->table[hashed_key];

    // Check if there is another hash node present already, if not, create a new one.
    if (current == NULL) {
        hnode_t *new_node = malloc(sizeof(*new_node));

        if (new_node == NULL) {
            LOG_ERROR("Error inserting key-value pair: memory allocation failed");
            return -1;
        }

        new_node->key = (void *) key;
        new_node->value = (void *) value;
        new_node->next = NULL;

        map->table[hashed_key] = new_node;
    }
    // Traverse the linked list.
    else {
        hnode_t *prev = NULL;
        // Check if the key already exists in the current linked list.
        do {
            if (!map->comp(current->key, key)) {
                // Update the value of the hash node.
                current->value = (void *) value;
                return 0;
            }
            // Move to the next hash node.
            prev = current;
            current = current->next;
        } while (current != NULL);

        // If the key does not exist in the linked list, create a new hash node.
        hnode_t *new_node = malloc(sizeof(*new_node));

        if (new_node == NULL) {
            LOG_ERROR("Error inserting key-value pair: memory allocation failed");
            return -1;
        }

        new_node->key = (void *) key;
        new_node->value = (void *) value;
        prev->next = new_node;
    }

    return 0;
}

/// @brief Remove a key-value pair from the hash map.
int htable_remove (htable_t *map, const void *key) {

    assert(map && map->table);

    // Calculate the hash index.
    const size_t hashed_key = get_hash(map, key);

    hnode_t *current = map->table[hashed_key];
    hnode_t *prev = NULL;

    while (current != NULL) {
        // Check if the key exists in the linked list and remove it.
        if (!map->comp(current->key, key)) {
            
            // Link the previous hash node to the next hash node.
            if (prev == NULL) {
                map->table[hashed_key] = current->next;
            } else {
                prev->next = current->next;
            }

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
void *htable_get (htable_t *map, const void *key) {
    
    assert(map && map->table);

    // Calculate the hash index.
    const size_t hashed_key = get_hash(map, key);

    hnode_t *current = map->table[hashed_key];

    // Traverse the linked list to find the hash node.
    while (current != NULL) {
        // Check if the key matches the current hash node.
        if (!map->comp(current->key, key)) {
            return current->value;
        }

        // Move to the next hash node.
        current = current->next;
    }

    return NULL;
}