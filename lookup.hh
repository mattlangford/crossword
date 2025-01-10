#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <array>
#include <cstddef>
#include <filesystem>
#include <map>

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
            add_to_cache(word, index);
            words_.push_back(std::move(word));
        }
        std::cout << "Loaded " << words_.size() << " words\n";
    }

    const std::vector<WordIndex>& words_with_characters_at(const LookupQuery& query, size_t opening) const {
        // static size_t counter_ = 0;
        // if (counter_++ % 100000 == 0) {
        //     std::cout << "Query with opening " << opening << "\n";
        //     for (size_t i = 0; i < query.size(); ++i) std::cout << " " << query[i].first << ": '" << query[i].second << "'\n";
        //     std::cout << "\n";
        // }
        if (query.empty()) {
            return words_by_length_[opening];
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
    static size_t to_index(char c) { return std::tolower(c) - 'a'; }

    void add_to_cache(const std::string& word, WordIndex index) {
        words_by_length_[word.size()].push_back(index);

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
    std::vector<std::string> words_;
    std::array<std::vector<WordIndex>, SIZE> words_by_length_;
    std::array<std::unordered_map<LookupQuery, std::vector<WordIndex>>, SIZE> cache_;
};

