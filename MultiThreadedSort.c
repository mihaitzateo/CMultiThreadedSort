#include <stdio.h>    // For standard input/output operations like printf
#include <stdlib.h>   // For malloc, free, rand, srand, qsort
#include <time.h>     // For clock_gettime, timespec, time (for srand)
#include <pthread.h>  // For pthread_create, pthread_join, pthread_t
#include <string.h>   // For memcpy

// Define the size of the array and the number of threads for multi-threading
#define ARRAY_SIZE 100000
#define NUM_SORT_THREADS 4
#define SUB_ARRAY_SIZE (ARRAY_SIZE / NUM_SORT_THREADS) // Each thread sorts 25,000 elements

// Structure to pass data to threads for sorting and merging
typedef struct {
    int *array;            // Pointer to the main array (source for sorting, source for merging)
    int start_index;       // Starting index of the sub-array for the thread
    int length;            // Length of the sub-array for the thread

    // Parameters specifically for merge operations
    int *temp_array;       // Pointer to a temporary array (destination for merged results)
    int merge_left_start;  // Starting index of the left sub-array to merge
    int merge_left_len;    // Length of the left sub-array
    int merge_right_start; // Starting index of the right sub-array to merge
    int merge_right_len;   // Length of the right sub-array
    int merge_target_start;// Starting index in the temp_array where merged results are stored
} ThreadData;

// Comparison function required by qsort for sorting integers in ascending order
int compare_integers(const void *a, const void *b) {
    // Cast void pointers to int pointers and dereference them for comparison
    return (*(int*)a - *(int*)b);
}

// Function to merge two sorted sub-arrays into a destination array.
// It merges:
//   'source_array[left_start ... left_start + left_len - 1]'
//   'source_array[right_start ... right_start + right_len - 1]'
// into:
//   'dest_array[target_start ... target_start + left_len + right_len - 1]'
void merge(int *source_array, int *dest_array, int left_start, int left_len,
           int right_start, int right_len, int target_start) {
    int i = 0; // Index for the left sub-array (relative to left_start)
    int j = 0; // Index for the right sub-array (relative to right_start)
    int k = 0; // Index for the destination array (relative to target_start)

    // Merge elements while both sub-arrays have elements remaining
    while (i < left_len && j < right_len) {
        if (source_array[left_start + i] <= source_array[right_start + j]) {
            dest_array[target_start + k++] = source_array[left_start + i++];
        } else {
            dest_array[target_start + k++] = source_array[right_start + j++];
        }
    }

    // Copy any remaining elements from the left sub-array
    while (i < left_len) {
        dest_array[target_start + k++] = source_array[left_start + i++];
    }

    // Copy any remaining elements from the right sub-array
    while (j < right_len) {
        dest_array[target_start + k++] = source_array[right_start + j++];
    }
}

// Thread function for the initial sorting phase.
// Each thread sorts a segment of the 'array' using qsort.
void *thread_sort_func(void *arg) {
    ThreadData *data = (ThreadData *)arg; // Cast the argument to ThreadData pointer
    // Call qsort on the specified sub-array
    qsort(data->array + data->start_index, data->length, sizeof(int), compare_integers);
    return NULL; // Threads return NULL upon completion
}

// Thread function for the merge phase.
// Each thread performs a merge operation as defined by its ThreadData.
void *thread_merge_func(void *arg) {
    ThreadData *data = (ThreadData *)arg; // Cast the argument to ThreadData pointer
    // Call the merge function with the parameters from ThreadData
    merge(data->array, data->temp_array,
          data->merge_left_start, data->merge_left_len,
          data->merge_right_start, data->merge_right_len,
          data->merge_target_start);
    return NULL; // Threads return NULL upon completion
}

// Helper function to get the current time in microseconds using CLOCK_MONOTONIC
// CLOCK_MONOTONIC provides a steadily increasing clock, not affected by system time changes.
long long get_time_in_microseconds() {
    struct timespec ts; // Structure to hold seconds and nanoseconds
    clock_gettime(CLOCK_MONOTONIC, &ts); // Get the current time
    // Convert seconds to microseconds and add nanoseconds (converted to microseconds)
    return (long long)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

int main() {
    // Allocate memory for the original array and two copies for sorting
    int *original_array = (int *)malloc(ARRAY_SIZE * sizeof(int));
    int *single_thread_array = (int *)malloc(ARRAY_SIZE * sizeof(int));
    int *multi_thread_array = (int *)malloc(ARRAY_SIZE * sizeof(int));
    // Allocate a temporary array, crucial for merge operations to store intermediate results
    int *temp_array = (int *)malloc(ARRAY_SIZE * sizeof(int));

    // Check if memory allocation was successful
    if (!original_array || !single_thread_array || !multi_thread_array || !temp_array) {
        perror("Failed to allocate memory"); // Print error if allocation fails
        return 1; // Exit with an error code
    }

    // Seed the random number generator using the current time
    srand(time(NULL));

    // Generate the original array with random integers
    printf("Generating an array of %d random integers...\n", ARRAY_SIZE);
    for (int i = 0; i < ARRAY_SIZE; i++) {
        original_array[i] = rand() % 1000000; // Generate numbers between 0 and 999,999
    }
    printf("Array generation complete.\n");

    // ---------------------------------------------------------------------
    // --- Single-threaded Quicksort ---
    // ---------------------------------------------------------------------
    printf("\n--- Single-threaded Quicksort ---\n");
    // Copy the original array to the single-threaded array
    memcpy(single_thread_array, original_array, ARRAY_SIZE * sizeof(int));

    // Get start time for single-threaded sort
    long long start_time_single = get_time_in_microseconds();
    // Perform quicksort on the entire array
    qsort(single_thread_array, ARRAY_SIZE, sizeof(int), compare_integers);
    // Get end time for single-threaded sort
    long long end_time_single = get_time_in_microseconds();
    long long elapsed_single = end_time_single - start_time_single;

    printf("Single-threaded sort completed in %lld microseconds.\n", elapsed_single);

    // ---------------------------------------------------------------------
    // --- Multi-threaded Quicksort with Merging ---
    // ---------------------------------------------------------------------
    printf("\n--- Multi-threaded Quicksort with Merging ---\n");
    // Copy the original array to the multi-threaded array
    memcpy(multi_thread_array, original_array, ARRAY_SIZE * sizeof(int));

    pthread_t threads[NUM_SORT_THREADS]; // Array to hold thread identifiers
    ThreadData thread_data[NUM_SORT_THREADS]; // Array to hold data for each thread

    // Get start time for multi-threaded process
    long long start_time_multi = get_time_in_microseconds();

    // 1. Initial sorting of sub-arrays using 4 threads
    printf("Stage 1: Sorting %d sub-arrays using %d threads (each %d elements)...\n",
           NUM_SORT_THREADS, NUM_SORT_THREADS, SUB_ARRAY_SIZE);
    for (int i = 0; i < NUM_SORT_THREADS; i++) {
        thread_data[i].array = multi_thread_array; // Each thread works on a segment of this array
        thread_data[i].start_index = i * SUB_ARRAY_SIZE;
        thread_data[i].length = SUB_ARRAY_SIZE;
        // Create a new thread and pass its data
        pthread_create(&threads[i], NULL, thread_sort_func, &thread_data[i]);
    }

    // Wait for all 4 sorting threads to complete
    for (int i = 0; i < NUM_SORT_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    printf("Stage 1 complete: All sub-arrays are sorted individually.\n");

    // 2. First level merging: two 50,000-element sub-arrays using 2 threads
    printf("Stage 2: Merging 2 pairs of 25,000-element sub-arrays into two 50,000-element blocks in temp_array using 2 threads...\n");

    // Thread 0: merges multi_thread_array[0...24999] and multi_thread_array[25000...49999]
    // into temp_array[0...49999]
    thread_data[0].array = multi_thread_array; // Source array for merging
    thread_data[0].temp_array = temp_array;    // Destination array for merged result
    thread_data[0].merge_left_start = 0;
    thread_data[0].merge_left_len = SUB_ARRAY_SIZE;
    thread_data[0].merge_right_start = SUB_ARRAY_SIZE;
    thread_data[0].merge_right_len = SUB_ARRAY_SIZE;
    thread_data[0].merge_target_start = 0;
    pthread_create(&threads[0], NULL, thread_merge_func, &thread_data[0]);

    // Thread 1: merges multi_thread_array[50000...74999] and multi_thread_array[75000...99999]
    // into temp_array[50000...99999]
    thread_data[1].array = multi_thread_array; // Source array for merging
    thread_data[1].temp_array = temp_array;    // Destination array for merged result
    thread_data[1].merge_left_start = 2 * SUB_ARRAY_SIZE;
    thread_data[1].merge_left_len = SUB_ARRAY_SIZE;
    thread_data[1].merge_right_start = 3 * SUB_ARRAY_SIZE;
    thread_data[1].merge_right_len = SUB_ARRAY_SIZE;
    thread_data[1].merge_target_start = 2 * SUB_ARRAY_SIZE; // Target starts at 50,000 for the second block
    pthread_create(&threads[1], NULL, thread_merge_func, &thread_data[1]);

    // Wait for the two merging threads to complete
    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
    printf("Stage 2 complete: Two 50,000-element sorted blocks created in temp_array.\n");

    // 3. Final merge: combining the two 50,000-element segments into a single 100,000-element array.
    // This is done by the main thread.
    printf("Stage 3: Merging the two 50,000-element blocks from temp_array into the final 100,000-element array (using main thread)...\n");
    // Merges temp_array[0...49999] and temp_array[50000...99999]
    // into multi_thread_array[0...99999]
    merge(temp_array, multi_thread_array,
          0, ARRAY_SIZE / 2,         // Left half of temp_array (length 50,000)
          ARRAY_SIZE / 2, ARRAY_SIZE / 2, // Right half of temp_array (length 50,000)
          0);                         // Target starts at 0 in multi_thread_array
    printf("Stage 3 complete: Final array is fully sorted.\n");

    // Get end time for multi-threaded process
    long long end_time_multi = get_time_in_microseconds();
    long long elapsed_multi = end_time_multi - start_time_multi;

    printf("Multi-threaded sort completed in %lld microseconds.\n", elapsed_multi);

    // ---------------------------------------------------------------------
    // --- Performance Comparison ---
    // ---------------------------------------------------------------------
    printf("\n--- Performance Comparison ---\n");
    printf("Single-threaded QuickSort Time: %lld microseconds\n", elapsed_single);
    printf("Multi-threaded QuickSort + MergeSort Time: %lld microseconds\n", elapsed_multi);

    // ---------------------------------------------------------------------
    // --- Free allocated memory ---
    // ---------------------------------------------------------------------
    free(original_array);
    free(single_thread_array);
    free(multi_thread_array);
    free(temp_array);

    return 0; // Indicate successful execution
}

