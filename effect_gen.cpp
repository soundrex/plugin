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

    repeat_rand (int a=0, int b=0) : dist_(a, b) {}

    template <typename foo>
    int dist(foo &gen, unsigned int bound) {
        if ((count++)%bound == 0)
            prev = dist_(gen);

        return prev;
    }
} xdist[num_channels], ydist[num_channels];

union fprintr {
    float f;
    uint8_t c[4];
    fprintr(float const fl): f(fl) {}
};

//////////////////////////// implementations //////////////////////////////
void set_rows(int const num) {
    num_rows = num;
    for (int i=0; i<num_channels; ++i)
        ydist[i] = repeat_rand(1, 2*num-1);
}

void set_cols(int const num) {
    num_cols = num;
    for (int i=0; i<num_channels; ++i)
        xdist[i] = repeat_rand(1, 2*num-1);
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
    out << "* f " << effect.volume << ' ';
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
            << (num_cols-1)*0.5*(1+cos(effect.theta))
            << " I % # h " << num_cols << " 2 h 2";
            break;
        case effect_t::Circular:
            out << "I < + p - I % # h " << num_cols << " f "
            << (num_cols-1) * (0.5 - 0.4*cos(effect.theta)) << " 2"
            " p - I % / # h " << num_cols << " h " << num_rows << " f "
            << (num_rows-1) * (0.5 - 0.4*sin(effect.theta)) << " 2"
            " h 5";
            break;
        case effect_t::Randomized:
            out << "I *"
            " < | - I % # h " << num_cols << " f " << xdist[effect.track_id].dist(gen, effect.timePeriod())*0.5 << " h 3"
            " < | - I % / # h " << num_cols << " h " << num_rows << " f "
            << ydist[effect.track_id].dist(gen, effect.timePeriod())*0.5 << "h 3";
            break;
        case effect_t::SpreadingOut:
            out << "*"
            " I < | - I % # h " << num_cols << " f " << (num_cols-1)*0.5 << " f "
            << (num_cols+1)*0.5*(0.3 + 0.7 * (packet_num - effect.init_packet) /effect.timePeriod())
            << " I < | - I / # h " << num_cols << " f " << (num_rows-1)*0.5 << " f "
            << (num_rows+1)*0.5*(0.3 + 0.7 * (packet_num - effect.init_packet)/effect.timePeriod());
            break;
        case effect_t::Diagonal:
            out << "e - h 0 / p - f "
            << (num_cols-1)*0.5*(1+sin(effect.theta))
            << " / + I % # h " << num_cols << " I % / # h " << num_cols << " h " << num_rows << " f 1.414 2 h 2";
            break;
        case effect_t::typeNum:; // For completeness
    }
    return out;
}
