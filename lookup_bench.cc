#include <random>
#include <iostream>
#include <format>

#include "lookup.hh"

std::mt19937_64 rng(42);

FlatVector<std::pair<size_t, char>, SIZE> generate_request(size_t opening) {
    std::uniform_int_distribution<int> dist('a', 'z');
    std::bernoulli_distribution b(0.5);

    FlatVector<std::pair<size_t, char>, SIZE> result;
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

    constexpr size_t TEST_RUNS = 100'000'000;

    struct Cases {
        size_t opening;
        FlatVector<std::pair<size_t, char>, SIZE> request;
    };
    std::vector<Cases> test_cases;
    for (size_t test_case = 0; test_case < 0.001 * TEST_RUNS; ++test_case) {
        size_t opening = opening_dist(rng);
        test_cases.push_back({opening, generate_request(opening)});
    }
    std::cout << test_cases.size() << " test cases generated\n";

    double avg = 0.0;

    using Timer = std::chrono::high_resolution_clock;
    const auto start = Timer::now();
    for (size_t run = 0; run < TEST_RUNS; ++run) {
        const auto& [opening, request] = test_cases[run % test_cases.size()];
        const auto indicies = lookup.words_with_characters_at(request, opening);
        avg += indicies.size();

        // std::cout << "Opening " << opening << ":\n";
        // for (size_t i = 0; i < request.size(); ++i) {
        //     auto [index, c] = request[i];
        //     std::cout << " index " << index << " c: " << c << "\n";
        // }
        // std::cout << "got " << indicies.size() << " results\n";
    }
    const auto stop = Timer::now();

    double rate = static_cast<double>(TEST_RUNS) / std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(stop - start).count();
    std::cout << std::format("lookup.words_with_characters_at at {:.2f}/ms with {:.2f} avg words\n", rate, avg / TEST_RUNS);

}