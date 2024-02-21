#include <bitset>
#include <iostream>
#include <random>
#include <time.h>
#include <vector>

//Only changes once every second, but this should be good enough random seeding for this code's purposes
std::mt19937::result_type get_seed() {
    return time(nullptr);
}

std::mt19937 get_seeded_rng() {
    std::mt19937 new_rng;
    const std::mt19937::result_type seedval = get_seed();
    new_rng.seed(seedval);
    return new_rng;
}

//I used a static function member instead of a global variable. I feel this makes the function more self-contained.
std::vector<unsigned char> get_random_bit_mask(float probability, int length) {
    static std::mt19937 rng = get_seeded_rng();
    std::bernoulli_distribution d(probability);
    std::vector<unsigned char> result = {};
    
    for (int i = 0; i < length; i++) {
        result.push_back(d(rng) + 2*d(rng) + 4*d(rng) + 8*d(rng) + 16*d(rng) + 32*d(rng) + 64*d(rng) + 128*d(rng));
    }
    
    return result;
}

//flips bits with a probability of the "probability" input parameter
void flip_random_bits(std::vector<unsigned char>& input_vector, float probability) {
    
    const std::vector<unsigned char> random_bit_mask = get_random_bit_mask(probability, input_vector.size());    
    
    for (int i = 0; i < input_vector.size(); i++) {
        input_vector[i] = input_vector[i] ^ random_bit_mask[i];
    }
}

bool get_random_bool(float probability) {
    static std::mt19937 rng = get_seeded_rng();
    std::bernoulli_distribution d(probability);
    return d(rng);
}
