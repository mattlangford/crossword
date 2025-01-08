#include <iostream>
#include <vector>
#include <string>
#include <format>
#include <array>
#include <sstream>
#include <map>
#include <random>
#include <set>
#include <fstream>

#include <filesystem>

#include "flat_vector.hh"
#include "lookup.hh"
#include "constants.hh"

class Board {
public:
    static constexpr char OPEN = ' ';
    static constexpr char BLOCKED = '#';

    using Index = uint16_t;

    Board() {
        for (auto& c : board_) c = OPEN;
    }

    void set_index(Index index, char c) { board_.at(index) = c; }
    void set(uint8_t row, uint8_t col, char c) { set_index(to_index(row, col), c); }
    void block(uint8_t row, uint8_t col) { set(row, col, BLOCKED); }
    char at_index(Index index) const { return board_.at(index); }
    char at(uint8_t row, uint8_t col) const { return at_index(to_index(row, col)); }
    void reset_nonblocked_to_open() {
        for (auto& c : board_) if (c != BLOCKED) c = OPEN;
    }

    std::string read(const std::vector<WordIndex>& index) const {
        std::string s;
        s.reserve(index.size());
        for (const WordIndex i : index) {
            s.push_back(board_[i]);
        }
        return s;
    }

    struct WordIndicies {
        std::map<size_t, std::vector<Index>> rows;
        std::map<size_t, std::vector<Index>> cols;
    };

    WordIndicies generate_word_index() const {
        WordIndicies result;

        Board col_index = *this;
        col_index.reset_nonblocked_to_open();
        Board row_index = col_index;
        size_t current_index = 1;

        auto set_if_open = [&](Index index, char c, Board& b) -> bool {
            if (b.at_index(index) != OPEN) {
                return false;
            }
            b.set_index(index, c);
            return true;
        };

        // Iteration order is important!
        for (uint8_t row = 0; row < DIM; ++row) {
            for (uint8_t col = 0; col < DIM; ++col) {
                const Index board_index = to_index(row, col);

                char c = at_index(board_index);
                if (c == Board::BLOCKED) {
                    continue;
                }

                bool used = false;
                if (set_if_open(board_index, current_index, col_index)) {
                    used = true;
                    result.cols[current_index].push_back(board_index);
                    for (uint8_t inner = row + 1; inner < DIM; ++inner) {
                        Index inner_index = to_index(inner, col);

                        if (!set_if_open(inner_index, '-', col_index)) break;
                        result.cols[current_index].push_back(inner_index);
                    }
                }
                if (set_if_open(board_index, current_index, row_index)) {
                    used = true;
                    result.rows[current_index].push_back(board_index);
                    for (uint8_t inner = col + 1; inner < DIM; ++inner) {
                        Index inner_index = to_index(row, inner);

                        if (!set_if_open(inner_index, '-', row_index)) break;
                        result.rows[current_index].push_back(inner_index);
                    }
                }

                if (used) {
                    current_index++;
                }
            }
        }

        return result;
    }

    std::string word(const std::vector<Index>& indicies) const {
        std::string s;
        for (const Index& i : indicies) s += board_.at(i);
        return s;
    }

    FlatVector<std::pair<WordIndex, char>, SIZE> get_characters_at(const std::vector<WordIndex>& indicies) const {
        FlatVector<std::pair<WordIndex, char>, SIZE> result;
        std::string w = word(indicies);
        for (WordIndex i = 0; i < w.size(); ++i) {
            char c = w[i];
            if (c == BLOCKED) throw std::runtime_error("Found blocked!");
            if (c != OPEN) {
                result.push_back(std::make_pair(i, c));
            }
        }
        return result;
    }

    std::string to_string() const {
        std::stringstream ss;
        ss << "  ";
        for (uint8_t col = 0; col < DIM; col++) ss << col;
        ss << "\n";
        for (uint8_t row = 0; row < DIM; row++) {
            ss << row << "|";
            for (uint8_t col = 0; col < DIM; col++) {
                ss << board_.at(to_index(row, col));
            }
            ss << "|\n";
        }
        for (uint8_t col = 0; col < DIM; col++) ss << "-";
        ss << "----";
        return ss.str();
    }

private:
    uint16_t to_index(uint8_t row, uint8_t col) const { return static_cast<uint16_t>(row) + col * DIM; }

    std::array<char, DIM * DIM> board_;
};

void write_ipuz(const Board& final_board, const Board::WordIndicies& index, const std::filesystem::path& output) {
    Board puzzle_board = final_board;
    puzzle_board.reset_nonblocked_to_open();
    for (const auto& [i, e] : index.rows) puzzle_board.set_index(e.front(), i);
    for (const auto& [i, e] : index.cols) puzzle_board.set_index(e.front(), i);

    std::ofstream file(output);
    if (!file.is_open()) throw std::runtime_error(std::format("Unable to open '{}' for writing", output.string()));
    file << "{\n";
    file << "  \"version\": \"http://ipuz.org/v2\",\n";
    file << "  \"kind\": \"http://ipuz.org/crofileword\",\n";
    file << std::format(R"(  "dimensions": {{"width": {}, "height": {}}},)", DIM, DIM);
    file << "\n  \"puzzle\": [\n";
    for (size_t row = 0; row < DIM; ++row) {
        file << "    [";
        for (size_t col = 0; col < DIM; ++col) {
            char c = puzzle_board.at(row, col);
            if (c == Board::BLOCKED) file << "\"#\"";
            else if (c == Board::OPEN) file << 0;
            else file << (size_t)c;
            if (col != DIM - 1) file << ",";
        }
        file << "]";
        if (row != DIM - 1) file << ",";
        file << "\n";
    }
    file << "  ],\n  \"solution\": [\n";
    for (size_t row = 0; row < DIM; ++row) {
        file << "    [";
        for (size_t col = 0; col < DIM; ++col) {
            char c = final_board.at(row, col);
            file << '"' << (char)std::toupper(c) << '"';
            if (col != DIM - 1) file << ",";
        }
        file << "]";
        if (row != DIM - 1) file << ",";
        file << "\n";
    }
    file << "  ],\n  \"clues\": {\n    \"Across\": [\n";
    for (const auto& [i, e] : index.rows) {
        file << "      [" << i << ", \"Clue for '" << final_board.read(e) << "'\"]";
        if (i != index.rows.rbegin()->first) file << ",";
        file << "\n";
    }
    file << "  ],\n  \"Down\": [\n";
    for (const auto& [i, e] : index.cols) {
        file << "      [" << i << ", \"Clue for '" << final_board.read(e) << "'\"]";
        if (i != index.cols.rbegin()->first) file << ",";
        file << "\n";
    }
    file << "    ]\n  }\n}";

    file.close();
}

static std::mt19937 gen(42);

int main() {
    Lookup lookup("/Users/mattlangford/Downloads/google-10000-english-usa.txt");

    Board b;
    b.block(0, 0);
    b.block(4, 4);

    std::cout << b.to_string() << "\n";
    const auto word_index = b.generate_word_index();

    const size_t total_words = word_index.rows.size() + word_index.cols.size();

    std::vector<const std::vector<WordIndex>*> to_visit;
    for (const auto& [_, e] : word_index.rows) to_visit.push_back(&e);
    for (const auto& [_, e] : word_index.cols) to_visit.push_back(&e);
    std::shuffle(to_visit.begin(), to_visit.end(), gen);

    struct Dfs {
        Board board;
        std::vector<const std::vector<WordIndex>*> to_visit;
        std::set<size_t> used;
    };

    std::uniform_int_distribution<> start_index_dist(0, 1000);

    size_t boards_checked = 0;
    using Timer = std::chrono::high_resolution_clock;
    const Timer::time_point start = Timer::now();
    Timer::time_point previous = start;

    std::stack<Dfs> dfs;
    dfs.push(Dfs{b, std::move(to_visit), {}});
    while (!dfs.empty()) {
        boards_checked++;

        const Timer::time_point now = Timer::now();
        if (now - previous > std::chrono::seconds(5)) {
            std::cout << std::format("Tested {} boards at {:.2f}/ms\n", boards_checked,
                boards_checked / std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(now - start).count());
            previous = now;
        }

        auto [board, to_visit, used] = std::move(dfs.top());
        dfs.pop();

        std::cout << "\n";
        std::cout << board.to_string() << "\n";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (to_visit.empty()) {
            std::cout << "\nDONE\n";
            std::cout << board.to_string() << "\n";
            std::filesystem::path path = std::format("/tmp/crossword_{}x{}_{}.ipuz", DIM, DIM, boards_checked);
            write_ipuz(board, word_index, path);
            std::cout << "Wrote data to " << path.string() << "\n\n";
            continue;
        }

        const std::vector<WordIndex>& indicies = *to_visit.back();

        // Don't worry about single characters
        if (indicies.size() == 1) {
            auto next_to_visit = to_visit;
            next_to_visit.pop_back();
            dfs.push({std::move(board), std::move(next_to_visit), std::move(used)});
            continue;
        }

        const auto positions = board.get_characters_at(indicies);
        const auto& candidates = positions.empty() ? 
            lookup.words_of_length(indicies.size()) :
            lookup.words_with_characters_at(positions, indicies.size());
        size_t start_index = start_index_dist(gen);
        for (size_t i = 0; i < candidates.size(); ++i) {
            const size_t index = candidates[(start_index + i) % candidates.size()];
            if (used.contains(index)) {
                continue;
            }

            auto next_to_visit = to_visit;
            next_to_visit.pop_back();

            const std::string& candidate = lookup.word(index);
            auto new_used = used;
            new_used.insert(index);

            Board new_board = board;
            if (indicies.size() != candidate.size()) throw std::runtime_error("Invalid candidate length");
            for (size_t j = 0; j < indicies.size(); ++j) {
                new_board.set_index(indicies[j], candidate[j]);
            }
            dfs.push({std::move(new_board), std::move(next_to_visit), std::move(new_used)});
        }
    }

    return 0;
}