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

#ifndef HTABLE_H_
#define HTABLE_H_

    // --- Standard Libraries --- //

    #include <stddef.h>     // For NULL definition.
    #include <stdlib.h>     // For memory allocation oeprations, e.g. malloc(3), free(3).

    // --- TypeDefs --- //

    typedef unsigned long (*htable_hash_t)(const void *key);
    typedef int (*htable_keq_t)(const void *keyA, const void *keyB);

    typedef void *(*htable_cpy_t)(const void *src);
    typedef void (*htable_free_t)(void *src);

    /// @brief Hash node structure.
    struct htable_node {
        void *key;              // The key for the hash node.
        void *value;            // The value for the hash node.
        struct htable_node *next; // The next hash node in the linked list.
    };

    struct callbacks {
        htable_cpy_t kcpy;
        htable_cpy_t vcpy;
        htable_free_t kfree;
        htable_free_t vfree;
    };

    /// @brief Hash table structure.
    typedef struct hash_map {
        struct htable_node **table; // The hash table.
        size_t size;                // The size of the hash table.
        size_t count;               // The number of elements in the hash table.
        htable_hash_t hash;         // The hash function for the keys.
        htable_keq_t keq;           // The comparison function for the keys.
        struct callbacks cbs;       // The callback functions for the hash table.
    } htable_t;

    // --- Function Prototypes --- //

    /// @brief Create a hash table with the specified size.
    /// @param size Total number of nodes in the hash table.
    /// @param hash User-defined hash function for the keys.
    /// @param keq User-defined comparison function for the keys.
    /// @return Pointer to the allocated hash table, NULL on failure.
    htable_t *htable_create (size_t size, htable_hash_t hash, htable_keq_t keq, const struct callbacks *cbs);

    /// @brief Destroy the hash table and free resources.
    /// @param table The hash table to destroy.
    void htable_destroy (htable_t *table);

    /// @brief Insert a key-value pair into the hash table.
    /// @param table The hash table to insert the key-value pair into.
    /// @param key The key for the hash node.
    /// @param value The value for the hash node.
    /// @return 0 on success, -1 on invalid input, -2 on memory allocation failure.
    int htable_insert (htable_t *table, const void *key, const void *value);

    /// @brief Remove a key-value pair from the hash table.
    /// @param table The hash table to remove the key-value pair from.
    /// @param key The key for the hash node.
    /// @return 0 on success, -1 on failure.
    int htable_remove (htable_t *table, const void *key);

    /// @brief Retrieve a value from the hash table.
    /// @param table The hash table to retrieve the value from.
    /// @param key The key for the hash node.
    /// @return Pointer to the value on success, NULL on failure.
    void *htable_get (htable_t *table, const void *key);

#endif // HASH_MAP_H_