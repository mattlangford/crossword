#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <format>
#include <array>
#include <sstream>
#include <map>
#include <random>
#include <set>
#include <fstream>
#include <thread>

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

    void set_index(Index index, char c) {
        if (index >= board_.size()) throw std::runtime_error("Trying to index outside of the board");
        board_[index] = c;
    }
    void set(uint8_t row, uint8_t col, char c) { set_index(to_index(row, col), c); }
    void block(uint8_t row, uint8_t col) { set(row, col, BLOCKED); }
    char at_index(Index index) const { return board_.at(index); }
    char at(uint8_t row, uint8_t col) const { return at_index(to_index(row, col)); }
    void reset_nonblocked_to_open() {
        for (auto& c : board_) if (c != BLOCKED) c = OPEN;
    }

    std::string read(const std::vector<Index>& index) const {
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

    FlatVector<std::pair<WordIndex, char>, SIZE> get_characters_at(const std::vector<Index>& indicies) const {
        FlatVector<std::pair<WordIndex, char>, SIZE> result;
        for (WordIndex i = 0; i < indicies.size(); ++i) {
            char c = board_[indicies[i]];
            if (c != OPEN) {
                if (c == BLOCKED) throw std::runtime_error("Found blocked!");
                result.push_back(std::make_pair(i, c));
            }
        }
        return result;
    }

    std::string to_string() const {
        std::stringstream ss;
        ss << "  ";
        for (uint8_t col = 0; col < DIM; col++) ss << (int)col;
        ss << "\n";
        for (uint8_t row = 0; row < DIM; row++) {
            ss << (int) row << "|";
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


class DfsHelper {
private:
    struct Dfs {
        Board board;

        uint16_t used_words = 0;

        union Contained {
            const std::vector<Board::Index>* to_visit;
            WordIndex word;
        };
        std::vector<Contained> contained;

        std::string to_string() {
            std::string s = board.to_string();
            s += std::format("\n used words: {}/{}", used_words, contained.size());
            return s;
        }
    };

public:
    DfsHelper(Board b, const std::vector<const std::vector<Board::Index>*>& to_visit) {
        Dfs d;
        d.board = b;
        for (const auto& w : to_visit) {
            d.contained.push_back({.to_visit=w});
        }
        data_.push(std::move(d));
    }

    bool done() const { return data_.empty(); }

    const std::vector<Board::Index>* indicies(const Dfs& current) const {
        if (current.used_words >= current.contained.size()) return nullptr;
        return current.contained[current.used_words].to_visit;
    }

    bool used_word(const Dfs& current, size_t candidate) const {
        for (uint16_t i = 0; i < current.used_words; ++i) {
            if (current.contained[i].word == candidate) return true;
        }
        return false;
    }

    const Board& board(const Dfs& current) const {
        return current.board;
    }

    void push(Dfs current, Board new_board, size_t index) {
        current.board = std::move(new_board);
        current.contained[current.used_words++].word = index;
        data_.push(std::move(current));
    }

    std::optional<Dfs> pop() {
        if (data_.empty()) return std::nullopt;
        Dfs next = std::move(data_.top());
        data_.pop();
        return next;
    }

private:
    std::stack<Dfs> data_;
};

static std::mt19937 gen(123);
static std::uniform_int_distribution<> start_index_dist(0, 1000);
using Timer = std::chrono::high_resolution_clock;
static Timer::time_point start;

std::vector<const std::vector<Board::Index>*> alternating_shuffle(const Board::WordIndicies& word_index) {
    std::vector<const std::vector<Board::Index>*> rows;
    rows.reserve(word_index.rows.size());
    for (const auto& [_, e] : word_index.rows) rows.push_back(&e);
    std::vector<const std::vector<Board::Index>*> cols;
    cols.reserve(word_index.cols.size());
    for (const auto& [_, e] : word_index.cols) cols.push_back(&e);

    std::shuffle(rows.begin(), rows.end(), gen);
    std::shuffle(cols.begin(), cols.end(), gen);

    std::vector<const std::vector<Board::Index>*> result;
    result.reserve(rows.size() + cols.size());
    std::bernoulli_distribution dist(0.5);
    bool rows_first = dist(gen);
    size_t i = 0;
    size_t j = 0;
    const auto& vec1 = rows_first ? rows : cols;
    const auto& vec2 = rows_first ? cols : rows;
    while (i < vec1.size() || j < vec2.size()) {
        if (i < rows.size()) {
            result.push_back(vec1[i++]);
        }
        if (j < vec2.size()) {
            result.push_back(vec2[j++]);
        }
    }
    return result;
}

void run(
    const std::string& name,
    Board b,
    const Board::WordIndicies& word_index,
    const Lookup& lookup,
    std::atomic<bool>& should_print) {

    std::vector<const std::vector<Board::Index>*> to_visit = alternating_shuffle(word_index);
    DfsHelper dfs_helper(b, to_visit);

    size_t boards_checked = 0;
    Timer::time_point previous = start;

    size_t start_index = start_index_dist(gen);
    while (auto current = dfs_helper.pop()) {
        boards_checked++;

        if (boards_checked % 100000 == 0) {
            bool expected = true;
            if (should_print.compare_exchange_weak(expected, false)) {
                auto now = Timer::now();
                std::cout << name << " =======\n";
                std::cout << std::format("Tested {} words at {:.2f}/ms\n", boards_checked,
                    boards_checked / std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(now - start).count());
                previous = now;

                std::cout << "\n" << current->to_string() << "\n\n";
            }
        }

        const std::vector<Board::Index>* indicies = dfs_helper.indicies(*current);

        // std::cout << "\n";
        // std::cout << dfs_helper.board(*current).to_string() << "\n";
        // std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (indicies == nullptr) {
            std::cout << "\nDONE\n";
            std::cout << dfs_helper.board(*current).to_string() << "\n";
            std::filesystem::path path = std::format("/tmp/crossword_{}x{}_{}_{}.ipuz", DIM, DIM, boards_checked, name);
            write_ipuz(dfs_helper.board(*current), word_index, path);
            std::cout << "Wrote data to " << path.string() << "\n\n";
            dfs_helper.pop();
            continue;
        }

        const auto positions = dfs_helper.board(*current).get_characters_at(*indicies);
        const auto& candidates = lookup.words_with_characters_at(positions, indicies->size());
        size_t this_start_index = start_index++;
        for (size_t i = 0; i < candidates.size(); ++i) {
            const size_t index = candidates[(this_start_index + i) % candidates.size()];
            if (dfs_helper.used_word(*current, index)) {
                continue;
            }

            const std::string& candidate = lookup.word(index);
            Board new_board = dfs_helper.board(*current);
            if (indicies->size() != candidate.size()) {
                throw std::runtime_error(std::format("Invalid candidate length {} != {}", indicies->size(), candidate));
            }
            for (size_t j = 0; j < indicies->size(); ++j) {
                new_board.set_index((*indicies)[j], candidate[j]);
            }

            if (i == candidates.size() - 1) {
                dfs_helper.push(std::move(*current), new_board, index);
            } else {
                dfs_helper.push(*current, new_board, index);
            }
        }
    }
    std::cout << name << " is done!\n";
}

int main() {
    Lookup lookup("/Users/mattlangford/Downloads/words_alpha.txt");

    Board b;
    b.block(0, 5);
    b.block(1, 5);

    b.block(4, 0);
    b.block(4, 8);

    b.block(7, 3);
    b.block(8, 3);

    b.block(3, 3);
    b.block(3, 4);
    b.block(4, 4);
    b.block(5, 4);
    b.block(5, 5);

    const auto word_index = b.generate_word_index();

    start = Timer::now();

    std::atomic<bool> should_print = false;
    std::vector<std::thread> threads;
    std::vector<bool> done;

    for (size_t thread = 0; thread < NUM_THREADS; ++thread) {
        done.push_back(false);
        std::string name = "thread" + std::to_string(thread);
        std::cout << "Spawning " << name << "\n";
        threads.push_back(std::thread([&, name](){
            run(name, b, word_index, lookup, should_print);
            done[thread] = true;
        }));
    }

    std::cout << b.to_string() << "\n";

    while (!std::all_of(done.begin(), done.end(), [](bool b){ return b; })) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        should_print = true;
    }

    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }

    return 0;
}