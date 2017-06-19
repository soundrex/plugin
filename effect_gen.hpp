#ifndef effect_gen_hpp
#define effect_gen_hpp

#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

constexpr int num_skip = 3;
extern uint32_t packet_num;

void set_rows(int const);
void set_cols(int const);
void write_to_vec(char const *effect_str, std::vector<uint8_t> &vec);

struct effect_t {
    enum type_t {
        Immersive,
        LeftHalf,
        RightHalf,
        LeftToRight,
        Circular,
        Randomized,
        SpreadingOut,
        Diagonal,
        typeNum
    } type;
    double timePeriod_s;

    inline double timePeriod() const {
        return 62.5 / num_skip * timePeriod_s; // 62.5 == 24000/384.
    }

    inline double omega() const {
        return 2 * M_PI / timePeriod();
    }
};

std::ostream& operator<< (std::ostream &, effect_t const &);

#endif /* effect_gen_hpp */
