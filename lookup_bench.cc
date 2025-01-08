#include <random>
#include <iostream>
#include <format>

#include "lookup.hh"

std::mt19937_64 rng(42);

FlatVector<std::pair<WordIndex, char>, SIZE> generate_request(size_t opening) {
    std::uniform_int_distribution<int> dist('a', 'z');
    std::bernoulli_distribution b(0.6);

    FlatVector<std::pair<WordIndex, char>, SIZE> result;
    for (size_t i = 0; i < opening; ++i) {
        if (b(rng)) {
            result.push_back(std::make_pair(i, dist(rng)));
        }
    }
    if (result.empty()) result.push_back(std::make_pair(opening / 2, dist(rng)));
    return result;
}

int main() {
    Lookup lookup("/Users/mattlangford/Downloads/google-10000-english-usa.txt");

    std::uniform_int_distribution<size_t> opening_dist(2, DIM);

    constexpr size_t TEST_RUNS = 10000000;

    struct Cases {
        size_t opening;
        FlatVector<std::pair<WordIndex, char>, SIZE> request;
    };
    std::vector<Cases> test_cases;
    for (size_t test_case = 0; test_case < 0.003 * TEST_RUNS + 2; ++test_case) {
        size_t opening = opening_dist(rng);
        test_cases.push_back({opening, generate_request(opening)});
    }
    std::cout << test_cases.size() << " test cases generated\n";

    std::vector<std::vector<WordIndex>> expected;
    expected.push_back({751, 1864});
    expected.push_back({3462});
    expected.push_back({352, 631, 835, 929, 980, 1072, 1566, 1598, 1740, 1902, 1984, 2024, 2061, 2063, 2299, 2335, 2564, 2788, 3059});
    expected.push_back({});
    expected.push_back({36, 79, 627, 709, 743, 746, 750, 777, 835, 1016, 1181, 1207, 1308, 1530, 1569, 1726, 2112, 2770, 2775, 3273, 3294, 3307, 3360});

    double avg = 0.0;

    using Timer = std::chrono::high_resolution_clock;
    const auto start = Timer::now();
    for (size_t run = 0; run < TEST_RUNS; ++run) {
        const auto& [opening, request] = test_cases[run % test_cases.size()];
        const auto indicies = lookup.words_with_characters_at(request, opening);

        // std::cout << "run " << run << ": " << "\n";
        // for (auto i : indicies) std::cout << i << ", ";
        // std::cout << "\n";

        if (run < expected.size()) {
            if (expected[run] != indicies) {
                throw std::runtime_error("Not matching expected!");
            }
        }
        avg += indicies.size();
    }
    const auto stop = Timer::now();

    double rate = static_cast<double>(TEST_RUNS) / std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(stop - start).count();
    std::cout << std::format("lookup.words_with_characters_at at {:.2f}/ms with {:.2f} avg words\n", rate, avg / TEST_RUNS);

}