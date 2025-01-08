#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <array>
#include <cstddef>
#include <filesystem>
#include <map>
// #include <arm_neon.h>

#include "constants.hh"
#include "flat_vector.hh"

using LookupQuery = FlatVector<std::pair<WordIndex, char>, SIZE>;
namespace std {
template<>
struct hash<LookupQuery> {
size_t operator()(const LookupQuery& q) const {
    static const size_t fnv_offset = 1469598103934665603ULL;
    static const size_t fnv_prime  = 1099511628211ULL;
    size_t h = fnv_offset;
    for (size_t i = 0; i < q.size(); ++i) {
        h ^= std::hash<size_t>()(q[i].first);
        h *= fnv_prime;
        h ^= std::hash<char>()(q[i].second);
        h *= fnv_prime;
    }
    return h;
}
};
}

std::ostream& operator<<(std::ostream& os, const LookupQuery& q) {
    os << "Query [";
    for (size_t i = 0; i < q.size(); ++i) {
        if (i != 0) {
            os << ", ";
        }
        os << "(" << q[i].first << ": " << q[i].second << ")";
    }
    os << "]";
    return os;
}

void logical_and_inplace(std::vector<WordIndex>& lhs, const std::vector<WordIndex>& rhs) {
    if (rhs.empty()) {
        lhs.clear();
        return;
    }

    auto l_it = lhs.begin();
    auto r_it = rhs.begin();

    while (l_it != lhs.end()) {
        auto next_it = l_it;
        for (; next_it != lhs.end(); ++next_it) {
            if (*next_it >= *r_it) {
                break;
            }
        }
        l_it = lhs.erase(l_it, next_it);

        if (*l_it == *r_it) {
            l_it++;
            r_it++;
        } else {
            // constexpr uint8_t WIDTH = 8;
            // while (std::distance(r_it, rhs.end()) >= WIDTH) {
            //     uint16x8_t scalar_vec = vdupq_n_u16(*l_it);
            //     uint16x8_t cmp = vcltq_u16(vld1q_u16(&*r_it), scalar_vec);
            //     uint64_t bitmask = vget_lane_u64(vreinterpret_u64_u8(vmovn_u16(cmp)), 0);
            //     size_t count = __builtin_popcountll(bitmask) / WIDTH;
            //     r_it += count;
            //     if (count != WIDTH) break;
            // }
            for (; r_it != rhs.end(); ++r_it) {
                if (*l_it <= *r_it) {
                    break;
                }
            }
        }

        if (r_it == rhs.end()) {
            lhs.erase(l_it, lhs.end());
            break;
        }
    }
}

class Lookup {
public:
    Lookup(std::filesystem::path fname) {
        std::ifstream file(fname);
        std::string word;

        while (file >> word) {
            if (word.size() <= 1 || word.size() > DIM) {
                continue;
            }
            size_t index = words_.size();
            WordsSplitByCharacterAtPosition& entry = by_length_.at(word.size());
            entry.all_words.push_back(index);

            if (entry.matricies.empty()) {
                entry.matricies.resize(word.size(), std::vector<WordsSplitByCharacterAtPosition::WordMatrix>(word.size()));
            }

            for (size_t l = 0; l < word.size(); ++l) {
                entry.by_char_pos[l][to_index(word[l])].push_back(index);

                for (size_t l2 = l + 1; l2 < word.size(); ++l2) {
                    entry.matricies.at(l).at(l2).at(to_index(word[l])).at(to_index(word[l2])).push_back(index);
                }
            }
            add_to_cache(word, index);
            words_.push_back(std::move(word));
        }
        std::cout << "Loaded " << words_.size() << " words\n";
    }

    const std::vector<WordIndex>& words_of_length(size_t length) const {
        return by_length_[length].all_words;
    }

    const std::vector<WordIndex>& words_with_characters_at(const LookupQuery& query, size_t opening) const {
        static size_t counter_ = 0;
        // if (counter_++ % 100000 == 0) {
        //     std::cout << "Query with opening " << opening << "\n";
        //     for (size_t i = 0; i < query.size(); ++i) std::cout << " " << query[i].first << ": '" << query[i].second << "'\n";
        //     std::cout << "\n";
        // }
        if (query.empty()) {
            return words_of_length(opening);
        }
        const auto& cache = cache_[opening];
        auto it = cache.find(query);
        if (it == cache.end()) {
            static std::vector<WordIndex> empty;
            return empty;
        }
        return it->second;
    }

    const std::string& word(size_t index) const { return words_.at(index); }

private:
    const std::vector<WordIndex>& words_with_character_at(size_t pos, char c, size_t opening) const {
        return by_length_.at(opening).by_char_pos.at(pos).at(to_index(c));
    }

    static size_t to_index(char c) { return std::tolower(c) - 'a'; }

    struct WordsSplitByCharacterAtPosition {
        using WordsSplitByCharacter = std::array<std::vector<WordIndex>, 26>;
        std::array<WordsSplitByCharacter, SIZE> by_char_pos;
        std::vector<WordIndex> all_words;

        using WordMatrix = std::array<WordsSplitByCharacter, 26>;
        std::vector<std::vector<WordMatrix>> matricies;
    };
    std::array<WordsSplitByCharacterAtPosition, SIZE> by_length_;
    std::vector<std::string> words_;

    void add_to_cache(const std::string& word, WordIndex index) {
        struct Bfs {
            size_t level = 0;
            LookupQuery query;
        };
        auto& cache = cache_[word.size()];

        std::queue<Bfs> bfs;
        bfs.push({0, {}});
        while (!bfs.empty()) {
            auto [level, query] = std::move(bfs.front());
            bfs.pop();

            for (size_t i = level; i < word.size(); ++i) {
                LookupQuery next_query = query;
                next_query.push_back(std::make_pair(i, word[i]));
                cache[next_query].push_back(index);
                bfs.push({i + 1, std::move(next_query)});
            }
        }
    }
    std::array<std::unordered_map<LookupQuery, std::vector<WordIndex>>, SIZE> cache_;
};

