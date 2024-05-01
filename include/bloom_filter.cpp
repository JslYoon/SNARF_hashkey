#include <iostream>
#include <vector>
#include <functional>

class BloomFilter {
private:
    std::vector<bool> bits;
    int numHashes;

public:
    BloomFilter()  {}

    void BloomFilter_init(size_t size, int numHashes) {
        bits.resize(size); 
        this->numHashes = numHashes;
    }

    std::size_t hash(int n, size_t x) {
        std::hash<int> hasher;
        return (hasher(x) + n * hasher(x + 1)) % bits.size();
    }

    void add(size_t item) {
        for (int n = 0; n < numHashes; ++n) {
            bits[hash(n, item)] = true;
        }
    }

    bool possiblyContains(size_t item) {
        for (int n = 0; n < numHashes; ++n) {
            if (!bits[hash(n, item)]) {
                return false;
            }
        }
        return true;
    }

    int return_size() {
        size_t bits_size = bits.size();  // This is the number of bits

        size_t total_size_bytes = (bits_size / 8) + sizeof(numHashes);

        // If you want to be precise and count any overhead for bits not exactly divisible by 8, you can adjust:
        if (bits_size % 8 != 0) {
            total_size_bytes += 1;  // Add one more byte to account for any leftover bits
        }
        return total_size_bytes;
    }
};
