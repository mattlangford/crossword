#pragma once
#include <vector>
#include <string>
#include <array>
#include <cstddef>

#include "constants.hh"
#include "flat_vector.hh"

std::vector<size_t> logical_and(const std::vector<size_t>& lhs, const std::vector<size_t>& rhs) {
    size_t lhs_i = 0;
    size_t rhs_i = 0;

    std::vector<size_t> result;
    while (lhs_i < lhs.size() && rhs_i < rhs.size()) {
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

class Lookup {
public:
    Lookup(const std::vector<std::string>& words) {
        for (size_t i = 0; i < words.size(); ++i) {
            const std::string& word = words[i];
            if (word.size() > SIZE) {
                throw std::runtime_error("Word is too long");
            }
            WordsSplitByCharacterAtPosition& entry = by_length_.at(word.size());
            entry.all_words.push_back(i);

            for (size_t l = 0; l < word.size(); ++l) {
                entry.by_char_pos[l][to_index(word[l])].push_back(i);
            }
        }
    }

    std::vector<size_t> words_with_characters_at(const FlatVector<std::pair<size_t, char>, SIZE>& entries, size_t opening) {
        if (entries.empty()) {
            return by_length_[opening].all_words;
        }

        if (entries.size() == 1) {
            return words_with_character_at(entries[0].first, entries[0].second, opening);
        }

        std::vector<size_t> merged = logical_and(
            words_with_character_at(entries[0].first, entries[0].second, opening),
            words_with_character_at(entries[1].first, entries[1].second, opening)
        );
        for (size_t i = 2; i < entries.size(); i ++) {
            if (merged.empty()) {
                return {};
            }
            merged = logical_and(merged, words_with_character_at(entries[i].first, entries[i].second, opening));
        }
        return merged;
    }


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
};
