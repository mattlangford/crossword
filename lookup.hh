#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <array>
#include <cstddef>
#include <filesystem>
#include <arm_neon.h>

#include "constants.hh"
#include "flat_vector.hh"

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
            constexpr uint8_t WIDTH = 8;
            while (std::distance(r_it, rhs.end()) >= WIDTH) {
                uint16x8_t scalar_vec = vdupq_n_u16(*l_it);
                uint16x8_t cmp = vcltq_u16(vld1q_u16(&*r_it), scalar_vec);
                uint64_t bitmask = vget_lane_u64(vreinterpret_u64_u8(vmovn_u16(cmp)), 0);
                size_t count = __builtin_popcountll(bitmask) / WIDTH;
                r_it += count;
                if (count != WIDTH) break;
            }
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
            WordsSplitByCharacterAtPosition& entry = by_length_.at(word.size());
            entry.all_words.push_back(words_.size());

            for (size_t l = 0; l < word.size(); ++l) {
                entry.by_char_pos[l][to_index(word[l])].push_back(words_.size());
            }
            words_.push_back(std::move(word));
        }
        std::cout << "Loaded " << words_.size() << " words\n";
    }

    const std::vector<WordIndex>& words_of_length(size_t length) const {
        return by_length_[length].all_words;
    }

    std::vector<WordIndex> words_with_characters_at(const FlatVector<std::pair<WordIndex, char>, SIZE>& query, size_t opening) const {
        static size_t counter_ = 0;
        // if (counter_++ % 100000 == 0) {
        //     std::cout << "Query with opening " << opening << "\n";
        //     for (size_t i = 0; i < query.size(); ++i) std::cout << " " << query[i].first << ": '" << query[i].second << "'\n";
        //     std::cout << "\n";
        // }
        if (query.empty()) {
            return words_of_length(opening);
        }

        std::vector<WordIndex> merged = 
            words_with_character_at(query[0].first, query[0].second, opening);

        for (size_t i = 1; i < query.size(); i ++) {
            if (merged.empty()) {
                return {};
            }
            logical_and_inplace(merged, words_with_character_at(query[i].first, query[i].second, opening));
        }
        return merged;
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
    };
    std::array<WordsSplitByCharacterAtPosition, SIZE> by_length_;
    std::vector<std::string> words_;
};
