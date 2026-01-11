#include "state.hpp"

#include <algorithm>
#include <cstring>
#include <random>

namespace ttt::game {

Obstacle::Obstacle(int seq_len) {
  m_left_size = m_right_size = m_down_size = m_up_size = 0;
  m_moves = new char[seq_len + 1];
  std::mt19937 rng(std::random_device{}());
  /*
    0 - UP (U)
    1 - LEFT (L)
    2 - DOWN (D)
    3 - RIGHT (R)
  */
  std::uniform_int_distribution<int> dist(0, 3);
  int prev_number = -1;
  int cur_x = 0;
  int cur_y = 0;
  for (int i = 0; i < seq_len; i++) {
    int cur_number = dist(rng);
    while (cur_number % 2 == prev_number % 2) { // to not make backward moves
      cur_number = dist(rng);
    }
    switch (cur_number) {
      case 0:
        m_moves[i] = 'U';
        cur_y++;
        break;
      case 1:
        m_moves[i] = 'L';
        cur_x--;
        break;
      case 2:
        m_moves[i] = 'D';
        cur_y--;
        break;
      case 3:
        m_moves[i] = 'R';
        cur_x++;
        break;
      default:
        m_moves[i] = 0;
    }
    if (cur_x <= 0 && -cur_x > m_left_size) {
      m_left_size = -cur_x;
    }
    if (cur_x >= 0 && cur_x > m_right_size) {
      m_right_size = cur_x;
    }
    if (cur_y <= 0 && -cur_y > m_down_size) {
      m_down_size = -cur_y;
    }
    if (cur_y >= 0 && cur_y > m_up_size) {
      m_up_size = cur_y;
    }
    prev_number = cur_number;
  }
  m_moves[seq_len] = 0;
}

Obstacle::~Obstacle() {
  delete[] m_moves;
}

int Obstacle::get_moves_len() const {
  return strlen(m_moves);
}

int FieldBitmap::_insert_obstacle(const Obstacle& obstacle, int x, int y, int max_len) {
  int i = 0;
  char move = 0;
  int cur_x = x;
  int cur_y = y;
  int inserted = 0;
  bool f_done = false;
  while (!f_done && inserted < max_len) {
    Sign cur_state = get(cur_x, cur_y);
    if (cur_state != Sign::WALL) {
      inserted++;
    }
    _set_unsafe(cur_x, cur_y, Sign::WALL);
    move = obstacle.get_move(i++);
    switch (move) {
      case 'D':
        cur_y--;
        break;
      case 'U':
        cur_y++;
        break;
      case 'L':
        cur_x--;
        break;
      case 'R':
        cur_x++;
        break;
      default:
        f_done = true;
        break;
    }
  }
  return inserted;
}

FieldBitmap::FieldBitmap(int rows, int cols, float playable_part, int max_obstacle_len)
    : m_rows(rows), m_cols(cols), m_bitmap(0), m_playable_part(playable_part),
    m_max_obstacle_len(max_obstacle_len), m_wall_n(0) {
  m_bitmap = new char[_bitmap_size()];
  generate();
}

FieldBitmap::FieldBitmap(const FieldBitmap &other) : m_bitmap(0) {
  *this = other;
}

FieldBitmap::FieldBitmap(FieldBitmap &&other) : m_bitmap(0) {
  *this = std::move(other);
}

FieldBitmap::~FieldBitmap() { delete[] m_bitmap; }

FieldBitmap &FieldBitmap::operator=(const FieldBitmap &other) {
  if (this == &other)
    return *this;
  m_cols = other.m_cols;
  m_rows = other.m_rows;
  m_playable_part = other.m_playable_part;
  m_wall_n = other.m_wall_n;
  m_max_obstacle_len = other.m_max_obstacle_len;
  delete[] m_bitmap;
  m_bitmap = new char[_bitmap_size()];
  std::memcpy(m_bitmap, other.m_bitmap, _bitmap_size());
  return *this;
}

FieldBitmap &FieldBitmap::operator=(FieldBitmap &&other) {
  if (this == &other)
    return *this;
  m_cols = other.m_cols;
  m_rows = other.m_rows;
  delete[] m_bitmap;
  m_bitmap = other.m_bitmap;
  other.m_cols = other.m_rows = 0;
  other.m_bitmap = 0;
  return *this;
}

Sign FieldBitmap::get(int x, int y) const {
  if (!is_valid(x, y))
    return Sign::WALL;
  const int bit_no = (x + y * m_cols) * 2;
  const int byte_no = bit_no / 8;
  const char value = (m_bitmap[byte_no] >> (bit_no % 8)) & 0b11;
  return static_cast<Sign>(value);
}

bool FieldBitmap::is_valid(int x, int y) const {
  return !(x < 0 || x >= m_cols || y < 0 || y >= m_rows);
}

void FieldBitmap::_set_unsafe(int x, int y, Sign s) {
  const int bit_no = (x + y * m_cols) * 2;
  const int offset = bit_no % 8;
  char &byte = m_bitmap[bit_no / 8];
  byte &= ~(0b11 << offset);
  const int value = static_cast<int>(s);
  byte |= value << offset;
}

void FieldBitmap::set(int x, int y, Sign s) {
  if (!is_valid(x, y) || !(s == Sign::X || s == Sign::O)) {
    return;
  }
  _set_unsafe(x, y, s);
}

void FieldBitmap::_find_obstacle_place(const Obstacle& obstacle, int& x, int& y) const {
  std::mt19937 rng(std::random_device{}());
  std::uniform_int_distribution<int> x_dist(0, m_cols);
  std::uniform_int_distribution<int> y_dist(0, m_rows);
  int x_start = x_dist(rng);
  int y_start = y_dist(rng);
  for (int i = y_start; i < m_rows; i++) {
    for (int j = x_start; j < m_cols; j++) {
      int x_to_check = j - obstacle.get_lsize();
      int y_to_check = i - obstacle.get_dsize();
      bool f_fit = true;
      while (f_fit && x_to_check <= j + obstacle.get_rsize()) {
        while (f_fit && y_to_check <= i + obstacle.get_usize()) {
          if (!is_valid(x_to_check, y_to_check) || get(x_to_check, y_to_check) == Sign::WALL) {
            f_fit = false;
          }
          y_to_check++;
        }
        x_to_check++;
      }
      if (f_fit) {
        x = j;
        y = i;
        return;
      }
    }
  }
  x = -1;
  y = -1;
}

void FieldBitmap::generate() {
  m_wall_n = 0;
  std::memset(m_bitmap, 0, _bitmap_size());
  std::mt19937 rng(std::random_device{}());
  std::uniform_int_distribution<int> dist(0, m_max_obstacle_len);
  int place_to_fill = (int)round(m_cols * m_rows * (1.f - m_playable_part));
  while (place_to_fill > 0) {
    int tries_n = 0;
    while (tries_n < m_max_obstacle_len) {
      int cur_seq_len = dist(rng);
      Obstacle obstacle(cur_seq_len);
      int x = -1;
      int y = -1;
      _find_obstacle_place(obstacle, x, y);
      if (x >= 0 && y >= 0) {
        int inserted_n = _insert_obstacle(obstacle, x, y, place_to_fill);
        m_wall_n += inserted_n;
        place_to_fill -= inserted_n;
        break;
      }
      tries_n++;
    }
    if (tries_n == m_max_obstacle_len + 1) {
      return;
    }
  }
}

int FieldBitmap::_bitmap_size() const { return (m_rows * m_cols * 2 + 7) / 8; }

void State::_empty_state() {
  const int n_cells = m_opts.rows * m_opts.cols;
  const int max_possible_moves = n_cells - m_field.get_wall_cells_num();
  if (m_opts.max_moves == 0 || m_opts.max_moves > max_possible_moves) {
    m_opts.max_moves = max_possible_moves;
  }
  m_move_no = 0;
  m_player = Sign::X;
  m_status = Status::CREATED;
  m_winner = Sign::NONE;
}

State::State(const Opts &opts) : m_opts(opts),
m_field(opts.rows, opts.cols, opts.playable_part, opts.max_obstacle_len) {
  _empty_state();
}

void State::reset() {
  m_field.generate();
  _empty_state();
}

MoveResult State::process_move(Sign player, int x, int y) {
  if (m_status == Status::ENDED) {
    return MoveResult::ENDED;
  }
  if (player == Sign::NONE || player == Sign::WALL) {
    return MoveResult::ERROR;
  }
  if (player != m_player) {
    return MoveResult::DQ_OUT_OF_ORDER;
  }
  if (!_valid_coords(x, y)) {
    return MoveResult::DQ_OUT_OF_FIELD;
  }
  if (get_value(x, y) != Sign::NONE) {
    return MoveResult::DQ_PLACE_OCCUPIED;
  }
  _set_value(x, y, player);
  ++m_move_no;
  m_player = _opp_sign(player);
  const bool winning = _is_winning(x, y);
  if (m_status == Status::LAST_MOVE) {
    m_status = Status::ENDED;
    if (winning) {
      return MoveResult::DRAW;
    } else {
      m_winner = m_player;
      return MoveResult::WIN;
    }
  }
  m_status = Status::ACTIVE;
  if (winning) {
    if (m_move_no % 2 == 0) {
      m_status = Status::ENDED;
      m_winner = _opp_sign(m_player);
      return MoveResult::WIN;
    } else if (m_move_no >= m_opts.max_moves) {
      m_winner = _opp_sign(m_player);
      m_status = Status::ENDED;
      return MoveResult::WIN;
    } else {
      m_status = Status::LAST_MOVE;
      return MoveResult::OK;
    }
  }
  if (m_move_no >= m_opts.max_moves) {
    m_status = Status::ENDED;
    return MoveResult::DRAW;
  }
  return MoveResult::OK;
}

Sign State::get_value(int x, int y) const { return m_field.get(x, y); }

Status State::get_status() const { return m_status; }

Sign State::get_current_player() const { return m_player; }

int State::get_move_no() const { return m_move_no; }

const State::Opts &State::get_opts() const { return m_opts; }

Sign State::get_winner() const { return m_winner; }

bool State::_valid_coords(int x, int y) const { return m_field.is_valid(x, y); }

void State::_set_value(int x, int y, Sign sign) { m_field.set(x, y, sign); }

Sign State::_opp_sign(Sign player) {
  switch (player) {
  case Sign::X:
    return Sign::O;
  case Sign::O:
    return Sign::X;
  default:
    return Sign::NONE;
  }
}

bool State::_is_winning(int x, int y) {
  static const struct {
    int dx;
    int dy;
  } directions[] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
  for (const auto dir : directions) {
    for (int n = 0; n < m_opts.win_len; ++n) {
      bool has_x = false, has_o = false, has_none = false, has_wall = false;
      for (int i = 0; i < m_opts.win_len; ++i) {
        const int dn = n - i;
        switch (get_value(x + dir.dx * dn, y + dir.dy * dn)) {
        case Sign::X:
          has_x = true;
          break;
        case Sign::O:
          has_o = true;
          break;
        case Sign::NONE:
          has_none = true;
          break;
        case Sign::WALL:
          has_wall = true;
          break;
        default:
          // There was an error message here. It's gone now.
          break;
        }
      }
      if (!has_none && !has_wall && (has_x && !has_o || has_o && !has_x)) {
        return true;
      }
    }
  }
  return false;
}

}; // namespace ttt::game
