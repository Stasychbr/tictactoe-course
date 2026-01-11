#pragma once

namespace ttt::game {

enum class Status { CREATED, ACTIVE, LAST_MOVE, ENDED };

enum class MoveResult {
  OK,
  DRAW,
  WIN,
  ENDED,
  DQ_OUT_OF_FIELD,
  DQ_OUT_OF_ORDER,
  DQ_PLACE_OCCUPIED,
  ERROR,
};

inline bool is_dq(MoveResult r) {
  return r == MoveResult::DQ_OUT_OF_FIELD || r == MoveResult::DQ_OUT_OF_ORDER ||
         r == MoveResult::DQ_PLACE_OCCUPIED;
}

enum class Sign { 
  NONE,
  X,
  O,
  WALL
};

class Obstacle {
  char* m_moves;
  int m_left_size;
  int m_right_size;
  int m_up_size;
  int m_down_size;
public:
  Obstacle(int seq_len);
  Obstacle(const Obstacle& other) = delete;
  Obstacle& operator=(const Obstacle& other) = delete;
  int get_rsize() const {return m_right_size;}
  int get_lsize() const {return m_left_size;}
  int get_usize() const {return m_up_size;}
  int get_dsize() const {return m_down_size;}
  char get_move(int i) const {return m_moves[i];};
  int get_moves_len() const;
  ~Obstacle();
};

class FieldBitmap {
  char *m_bitmap;
  int m_rows;
  int m_cols;
  float m_playable_part;
  int m_wall_n;
  int m_max_obstacle_len;
public:
  FieldBitmap(int rows, int cols, float playable_part, int max_obstacle_len);
  FieldBitmap(const FieldBitmap &other);
  FieldBitmap(FieldBitmap &&other);
  ~FieldBitmap();

  FieldBitmap &operator=(const FieldBitmap &other);
  FieldBitmap &operator=(FieldBitmap &&other);

  void set(int x, int y, Sign s);
  
  void generate();

  Sign get(int x, int y) const;
  bool is_valid(int x, int y) const;
  int get_wall_cells_num() const {return m_wall_n;};

private:
  int _bitmap_size() const;
  void _set_unsafe(int x, int y, Sign s);
  void _find_obstacle_place(const Obstacle& obstacle, int& x, int& y) const;
  int _insert_obstacle(const Obstacle& obstacle, int x, int y, int max_len);
};

class State {
public:
  struct Opts {
    int rows;
    int cols;
    int win_len;
    int max_moves;
    float playable_part;
    int max_obstacle_len;
  };

private:
  Opts m_opts;

  FieldBitmap m_field;
  int m_move_no;
  Status m_status;
  Sign m_player;
  Sign m_winner;

public:
  State(const Opts &opts);
  State(const State &state) = default;
  ~State() = default;

  void reset();
  MoveResult process_move(Sign player, int x, int y);

  Sign get_value(int x, int y) const;
  Status get_status() const;
  Sign get_current_player() const;
  int get_move_no() const;
  const Opts &get_opts() const;
  Sign get_winner() const;

  State &operator=(const State &state) = default;


private:
  bool _valid_coords(int x, int y) const;
  void _set_value(int x, int y, Sign sign);
  Sign _opp_sign(Sign player);
  bool _is_winning(int x, int y);
  void _empty_state();
};
}; // namespace ttt::game
