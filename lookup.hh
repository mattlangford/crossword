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

    // std::cout << "lhs: ";
    // for (auto i : lhs) std::cout << i << " ";
    // std::cout << "\nrhs: ";
    // for (auto i : rhs) std::cout << i << " ";
    // std::cout << "\n";

    auto l_it = lhs.begin();
    auto r_it = rhs.begin();

    while (l_it != lhs.end()) {
        auto next_it = l_it;
        for (; next_it != lhs.end(); ++next_it) {
            if (*next_it >= *r_it) {
                break;
            }
        }
        if (l_it != next_it) {
            l_it = lhs.erase(l_it, next_it);
        }
        if (*l_it == *r_it) {
            l_it++;
            r_it++;
        } else {
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

void logical_and_inplace_simd(std::vector<WordIndex>& lhs, const std::vector<WordIndex>& rhs) {
    if (rhs.empty()) {
        lhs.clear();
        return;
    }

    std::cout << "lhs: ";
    for (auto i : lhs) std::cout << i << " ";
    std::cout << "\nrhs:";
    for (auto i : rhs) std::cout << i << " ";
    std::cout << "\n";

    auto l_it = lhs.begin();
    auto r_it = rhs.begin();

    constexpr uint8_t WIDTH = 8;
    while (l_it != lhs.end() && std::distance(r_it, rhs.end()) >= WIDTH) {
        uint16x8_t l_val = vdupq_n_u16(*l_it);
        uint16x8_t r_vec = vld1q_u16(&(*r_it));
        uint16x8_t gt_mask = vcgtq_u16(r_vec, l_val);

        /*
        for (; lane < WIDTH; ++lane) {
            if (elements[lane]) break;
        }
        std::cout << "Skip " << (int)lane << "\n";
        l_it = lhs.erase(l_it, l_it + lane);

        if (*l_it == *r_it) {
            l_it++;
        } else {
            std::advance(r_it, WIDTH - lane);
        }
        */
    }

    if (r_it == rhs.end()) {
        lhs.erase(l_it, lhs.end());
        return;
    }

    while (l_it != lhs.end()) {
        if (*l_it == *r_it) {
            l_it++;
            r_it++;
        } else if (*l_it < *r_it) {
            l_it = lhs.erase(l_it);
        } else {
            r_it++;
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

    std::vector<WordIndex> words_with_characters_at(const FlatVector<std::pair<WordIndex, char>, SIZE>& entries, size_t opening) const {
        if (entries.empty()) {
            return words_of_length(opening);
        }

        std::vector<WordIndex> merged = 
            words_with_character_at(entries[0].first, entries[0].second, opening);

        for (size_t i = 1; i < entries.size(); i ++) {
            if (merged.empty()) {
                return {};
            }
            logical_and_inplace(merged, words_with_character_at(entries[i].first, entries[i].second, opening));
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
