#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <algorithm>
#include <execution>

int main() {
    const size_t size = 100000;
    std::vector<int> numbers(size);

    // Seed the random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(1, 1000000);

    // Generate 100,000 random integers
    for (size_t i = 0; i < size; ++i) {
        numbers[i] = distrib(gen);
    }

    // Measure the time for parallel sort
    auto start_time = std::chrono::high_resolution_clock::now();

    // Sort the vector using the parallel unsequenced execution policy
    std::sort(std::execution::par_unseq, numbers.begin(), numbers.end());

    auto end_time = std::chrono::high_resolution_clock::now();

    // Calculate the duration in microseconds
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

    std::cout << "Time taken for parallel sort: " << duration.count() << " microseconds." << std::endl;

    return 0;
}
