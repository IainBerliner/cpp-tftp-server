#include <random>
#include <vector>

#include <time.h>

/*
//Only changes once every second, but this should be good enough random seeding for this code
//repository's purposes
*/
std::mt19937::result_type get_seed() {
    return time(nullptr);
}

std::mt19937 get_seeded_rng() {
    std::mt19937 new_rng;
    const std::mt19937::result_type seedval = get_seed();
    new_rng.seed(seedval);
    return new_rng;
}

std::vector<uint8_t> get_random_bit_mask(double probability, uint64_t length) {
    static std::mt19937 rng = get_seeded_rng();
    std::bernoulli_distribution d(probability);
    std::vector<uint8_t> result = {};

    for (uint64_t i = 0; i < length; i++) {
        result.push_back(d(rng) + 2*d(rng) + 4*d(rng) + 8*d(rng) + 16*d(rng) + 32*d(rng)
                         + 64*d(rng) + 128*d(rng));
    }
    
    return result;
}

void flip_random_bits(std::vector<uint8_t>& input_vector, double probability) {
    std::vector<uint8_t> random_bit_mask = get_random_bit_mask(probability, input_vector.size());    
    
    for (uint64_t i = 0; i < input_vector.size(); i++) {
        input_vector[i] = input_vector[i] ^ random_bit_mask[i];
    }
}

bool get_random_bool(double probability) {
    static std::mt19937 rng = get_seeded_rng();
    std::bernoulli_distribution d(probability);
    return d(rng);
}
