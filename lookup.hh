#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <array>
#include <cstddef>
#include <filesystem>

#include "constants.hh"
#include "flat_vector.hh"

std::vector<size_t> logical_and(const std::vector<size_t>& lhs, const std::vector<size_t>& rhs) {
    size_t lhs_i = 0;
    size_t rhs_i = 0;

    const size_t lhs_size = lhs.size();
    const size_t rhs_size = rhs.size();

    std::vector<size_t> result;
    while (lhs_i < lhs_size && rhs_i < rhs_size) {
        size_t l = lhs[lhs_i];
        size_t r = rhs[rhs_i];

        if (l == r) {
            result.push_back(l);
            lhs_i++;
            rhs_i++;
        } else if (l < r) {
            lhs_i++;
        } else {
            rhs_i++;
        }
    }
    return result;
}

void logical_and_inplace(std::vector<size_t>& lhs, const std::vector<size_t>& rhs) {
    if (rhs.empty()) {
        lhs.clear();
        return;
    }

    auto l_it = lhs.begin();
    auto r_it = rhs.begin();

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

    const std::vector<size_t>& words_of_length(size_t length) const {
        return by_length_[length].all_words;
    }

    std::vector<size_t> words_with_characters_at(const FlatVector<std::pair<size_t, char>, SIZE>& entries, size_t opening) const {
        if (entries.empty()) {
            return words_of_length(opening);
        }

        std::vector<size_t> merged = 
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
    const std::vector<size_t>& words_with_character_at(size_t pos, char c, size_t opening) const {
        return by_length_.at(opening).by_char_pos.at(pos).at(to_index(c));
    }

    static size_t to_index(char c) { return std::tolower(c) - 'a'; }

    struct WordsSplitByCharacterAtPosition {
        using WordsSplitByCharacter = std::array<std::vector<size_t>, 26>;
        std::array<WordsSplitByCharacter, SIZE> by_char_pos;
        std::vector<size_t> all_words;
    };
    std::array<WordsSplitByCharacterAtPosition, SIZE> by_length_;
    std::vector<std::string> words_;
};
