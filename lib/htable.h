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

#ifndef HTABLE_H_
#define HTABLE_H_

    // --- Standard Libraries --- //

    #include <stddef.h>     // For NULL definition.
    #include <stdio.h>      // For standard I/O operations, e.g. printf(3), perror(3).
    #include <stdlib.h>     // For memory allocation oeprations, e.g. malloc(3), free(3).
    #include <string.h>     // For string operations, e.g. memset(3), memcpy(3).
    #include <assert.h>     // For debugging, e.g. assert(3).

    // --- Project Libraries --- //

    #include "logging.h" // For logging functions, e.g. LOG_ERROR.

    // --- TypeDefs --- //

    typedef unsigned long (*hash_func_t)(const void *key);
    typedef int (*comp_func_t)(const void *keyA, const void *keyB);

    /// @brief Hash node structure.
    typedef struct hash_node {
        void *key;              // The key for the hash node.
        void *value;            // The value for the hash node.
        struct hash_node *next; // The next hash node in the linked list.
    } hnode_t;

    /// @brief Hash map structure.
    typedef struct hash_map {
        hnode_t **table;    // The hash table.
        size_t size;            // The size of the hash table.
        hash_func_t hash;   // The hash function for the hash map.
        comp_func_t comp;   // The comparison function for the hash map.          
    } htable_t;

    // --- Function Prototypes --- //

    /// @brief Create a hash map with the specified size.
    /// @param size Total number of nodes in the hash map.
    //
    /// @return Pointer to the allocated hash map.
    htable_t *htable_create (size_t size, hash_func_t hash, comp_func_t comp);

    /// @brief Destroy the hash map and free resources.
    /// @param map The hash map to destroy.
    void htable_destroy (htable_t *map);

    /// @brief Insert a key-value pair into the hash map.
    /// @param map The hash map to insert the key-value pair into.
    /// @param key The key for the hash node.
    /// @param value The value for the hash node.
    /// @return 0 on success, -1 on failure.
    int htable_insert (htable_t *map, const void *key, const void *value);

    /// @brief Remove a key-value pair from the hash map.
    /// @param map The hash map to remove the key-value pair from.
    /// @param key The key for the hash node.
    /// @return 0 on success, -1 on failure.
    int htable_remove (htable_t *map, const void *key);

    /// @brief Retrieve a value from the hash map.
    /// @param map The hash map to retrieve the value from.
    /// @param key The key for the hash node.
    /// @return Pointer to the value on success, NULL on failure.
    void *htable_get (htable_t *map, const void *key);

    static inline size_t get_hash (const htable_t *map, const void *key) {
        return map->hash(key) % map->size;
    }

#endif // HASH_MAP_H_