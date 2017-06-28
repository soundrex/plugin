#include "effect_gen.hpp"

#include <random>

//////////////////////////// helpers //////////////////////////////
int num_rows = 10, num_cols = 10;
std::default_random_engine gen(12345);
std::string const functions = "I#hif+-*/Mm=<%sctel|p:_";

constexpr bool is_numeric (char const c) {
    return c >= '0' && c <= '9';
}

struct repeat_rand {
    unsigned int count = 0, prev;
    std::uniform_int_distribution<int> dist_;

    repeat_rand (int a, int b) : dist_(a, b) {}

    template <typename foo>
    int dist(foo &gen, unsigned int bound) {
        if ((count++)%bound == 0)
            prev = dist_(gen);

        return prev;
    }
} xdist(1, 19), ydist(1, 19);

union fprintr {
    float f;
    uint8_t c[4];
    fprintr(float const fl): f(fl) {}
};

std::ostream& operator<< (std::ostream &out, fprintr const &fr) {
    for (int i=0; i<4; ++i)
        out << (uint16_t)fr.c[i] << ' ';
    return out;
}

//////////////////////////// implementations //////////////////////////////
void set_rows(int const num) {
    num_rows = num;
    ydist = repeat_rand(1, 2*num-1);
}

void set_cols(int const num) {
    num_cols = num;
    xdist = repeat_rand(1, 2*num-1);
}


void write_to_vec(char const *effect, std::vector<uint8_t> &vec) {
    do {
        if (is_numeric(*effect)) {
            vec.push_back(atoi(effect));
            while (is_numeric(*++effect));
        } else {
            vec.push_back(functions.find(*effect));
            if (*effect++ == 'f') {
                fprintr fp(atof(effect));
                do ++effect;
                while (is_numeric(*effect) || *effect == '.' || *effect=='-');
                vec.insert(vec.end(), fp.c, fp.c + 4);
            }
        }
        while (*effect == ' ') ++effect;
    } while (*effect);
}


std::ostream& operator<< (std::ostream &out, effect_t const &effect) {
    switch (effect.type) {
        case effect_t::Immersive:
            out << "h 1";
            break;
        case effect_t::LeftHalf:
            out << "I < I % # h " << num_cols << " h " << num_cols/2;
            break;
        case effect_t::RightHalf:
            out << "I < h " << num_cols/2-1 << " I % # h " << num_cols;
            break;
        case effect_t::LeftToRight:
            out << "e - h 0 / p - f "
            << fprintr((num_cols-1)*0.5*(1+sin(effect.omega()*packet_num)))
            << " I % # h " << num_cols << " 2 h 2";
            break;
        case effect_t::Circular:
            out << "I < + p - I % # h " << num_cols << " f "
            << fprintr((num_cols-1) * (0.5 - 0.4*cos(effect.omega()*packet_num))) << " 2"
            " p - I % / # h " << num_cols << " h " << num_rows << " f "
            << fprintr((num_cols-1) * (0.5 - 0.4*sin(effect.omega()*packet_num))) << " 2"
            " h 5";
            break;
        case effect_t::Randomized:
            out << "I *"
            " < | - I % # h " << num_cols << " f " << fprintr(xdist.dist(gen, (int)effect.timePeriod())*0.5) << " h 3"
            " < | - I % / # h " << num_cols << " h " << num_rows << " f "
            << fprintr(ydist.dist(gen, (int)effect.timePeriod())*0.5) << "h 3";
            break;
        case effect_t::SpreadingOut:
            out << "*"
            " I < | - I % # h " << num_cols << " f " << (num_cols-1)*0.5 << " f "
            << fprintr((num_cols+1)*0.5*(0.3 + 0.7*packet_num/effect.timePeriod()))
            << " I < | - I / # h " << num_cols << " f " << (num_rows-1)*0.5 << " f "
            << fprintr((num_rows+1)*0.5*(0.3 + 0.7*packet_num/effect.timePeriod()));
            break;
        case effect_t::Diagonal:
            out << "e - h 0 / p - f "
            << fprintr((num_cols-1)*0.5*(1+sin(effect.omega()*packet_num)))
            << " / + I % # h " << num_cols << " I % / # h " << num_cols << " h " << num_rows << " f 1.414 2 h 2";
            break;
        case effect_t::typeNum:; // For completeness
    }
    return out;
}