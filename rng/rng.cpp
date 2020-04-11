#include <eosio/crypto.hpp>
#include <eosio/eosio.hpp>
#include <eosio/print.hpp>
#include <eosio/system.hpp>

using namespace eosio;

class [[eosio::contract("rng")]] rng : public eosio::contract {
  int oracle_registration_delay = 10;
  int number_of_seconds_to_keep_random_numbers = 100;
  float participation_requirement_threshold = 0.5;

public:
  rng(name receiver, name code, datastream<const char*> ds): contract(receiver, code, ds) {}

  [[eosio::action]]
  void commitnumber(name user, eosio::checksum256 hash, uint32_t reveal_time_block) {
    uint32_t current_block = current_time_block();

    require_auth(user);

    oracles_index oracles(get_self(), get_first_receiver().value);
    auto oracles_iterator = oracles.find(user.value);

    bool is_valid_oracle =
      oracles_iterator != oracles.end() &&
      current_block - oracle_registration_delay > oracles_iterator -> reg_at_block_time;

    check(
      is_valid_oracle,
      "Commits can only be made by oracles that have been registered for " +
       std::to_string(oracle_registration_delay) +
       " time blocks."
    );

    uint32_t can_commit_at =
      oracles_iterator -> last_miss_timestamp + oracles_iterator -> misses_count * 1000;

    bool is_not_banned_from_committing = can_commit_at < current_time_block();

    check(
      is_not_banned_from_committing,
      "You have missed a reveal and will not be able to commit until " +
       std::to_string(can_commit_at) +
       " time block."
    );

    // Make sure that we are not committing to the current block.
    check(
      reveal_time_block > current_block,
      "Block Time must be in future."
    );

    committed_numbers_index committed_numbers(get_self(), get_first_receiver().value);
    auto committed_numbers_iterator = committed_numbers.find(user.value);

    if (committed_numbers_iterator == committed_numbers.end()) {
      committed_numbers.emplace(user, [&]( auto& row ) {
        row.key = user;
        row.hash = hash;
        row.reveal_time_block = reveal_time_block;
        row.revealed_number = 0;
      });
    } else {
      if (committed_numbers_iterator -> revealed_number == 0) {
        auto oracles_iterator = oracles.find(user.value);
        oracles.modify(oracles_iterator, user, [&]( auto& row ) {
          row.last_miss_timestamp = current_block;
          row.misses_count = oracles_iterator -> misses_count + 1;
        });
      } else {
        committed_numbers.modify(committed_numbers_iterator, user, [&]( auto& row ) {
          row.hash = hash;
          row.reveal_time_block = reveal_time_block;
          row.revealed_number = 0;
        });
      }
    }
  }

  [[eosio::action]]
  void revealnumber(name user, uint32_t reveal_time_block, uint64_t revealed_number) {
    uint32_t time_block = current_time_block();

    require_auth(user);

    committed_numbers_index committed_numbers(get_self(), get_first_receiver().value);

    auto committed_numbers_iterator = committed_numbers.find(user.value);

    // If a commit was not made, return an error.
    check(committed_numbers_iterator != committed_numbers.end(), "No commits were made.");

    // If before reveal time, return an error.
    check(time_block >= committed_numbers_iterator -> reveal_time_block, "Commit is revealed too early.");

    // If after reveal time, return an error.
    check(
      time_block <= (committed_numbers_iterator -> reveal_time_block + 3),
      "Commit has expired."
    );

    // If already revealed, return an error.
    check(committed_numbers_iterator -> revealed_number == 0, "Number has already been revealed.");

    std::string revealed_number_with_user_name = std::to_string((committed_numbers_iterator -> key).value) + std::to_string(revealed_number);
    char const *revealed_number_with_user_name_char = revealed_number_with_user_name.c_str();

    // will raise an error if invalid hash
    assert_sha256(revealed_number_with_user_name_char, revealed_number_with_user_name.length(), committed_numbers_iterator -> hash);

    committed_numbers.modify(committed_numbers_iterator, user, [&]( auto& row ) {
      row.revealed_number = revealed_number;
    });

    update_global_random_number(user, revealed_number, committed_numbers_iterator -> reveal_time_block);
  }

  [[eosio::action]]
  void regoracle(name user) {
    require_auth(user);

    oracles_index oracles(get_self(), get_first_receiver().value);
    oracles.emplace(_self, [&]( auto& row ) {
      row.key = user;
      row.reg_at_block_time = current_time_block();
      row.misses_count = 0;
    });

    // Update oracles count value.
    stats_index stats(get_self(), get_first_receiver().value);
    auto stats_iterator = stats.find(get_self().value);

    if (stats_iterator == stats.end()) {
      stats.emplace(_self, [&]( auto& row ) {
        row.key = get_self();
        row.oracles_count = 1;
      });
    } else {
      stats.modify(stats_iterator, _self, [&]( auto& row ) {
        row.oracles_count += 1;
      });
    }
  }

  [[eosio::action]]
  void unregoracle(name user) {
    require_auth(user);

    // Eventually, we need to prevent anyone that has missed a block in the last week from unregistering.

    remove_oracle_data(user);
  }

  [[eosio::action]]
  void cleannumbers() {
    require_auth(_self);

    uint32_t time_block = current_time_block();

    // Going through every single random_number and deleting old rows.
    random_numbers_index random_numbers(get_self(), get_first_receiver().value);

    auto random_numbers_iterator = random_numbers.begin();
    while (random_numbers_iterator != random_numbers.end()) {
      if (random_numbers_iterator -> time_block < time_block - number_of_seconds_to_keep_random_numbers) {
        random_numbers_iterator = random_numbers.erase(random_numbers_iterator);
      } else {
        random_numbers_iterator++;
      }
    }
  }

  [[eosio::action]]
  void resetdata() {
    require_auth(_self);

    oracles_index oracles(get_self(), get_first_receiver().value);

    // Going through every single oracle row and deleting them.
    auto oracles_iterator = oracles.begin();
    while (oracles_iterator != oracles.end()) {
      oracles_iterator = oracles.erase(oracles_iterator);
    }

    // Going through every single random_number row and deleting them.
    random_numbers_index random_numbers(get_self(), get_first_receiver().value);

    auto random_numbers_iterator = random_numbers.begin();
    while (random_numbers_iterator != random_numbers.end()) {
      random_numbers_iterator = random_numbers.erase(random_numbers_iterator);
    }

    committed_numbers_index committed_numbers(get_self(), get_first_receiver().value);

    // Going through every single committed_numbers row and deleting them.
    auto committed_numbers_iterator = committed_numbers.begin();
    while (committed_numbers_iterator != committed_numbers.end()) {
      committed_numbers_iterator = committed_numbers.erase(committed_numbers_iterator);
    }

    stats_index stats(get_self(), get_first_receiver().value);

    // Going through every single committed_numbers row and deleting them.
    auto stats_iterator = stats.begin();
    while (stats_iterator != stats.end()) {
      stats_iterator = stats.erase(stats_iterator);
    }
  }

private:
  void update_global_random_number(name user, uint64_t random_number, uint32_t current_time_block) {
    random_numbers_index random_numbers(get_self(), get_first_receiver().value);
    auto random_numbers_iterator = random_numbers.find(current_time_block);

    // Updating the random number value for the current block.
    if (random_numbers_iterator == random_numbers.end()) {
      random_numbers.emplace(_self, [&]( auto& row ) {
        row.random_number = random_number;
        row.time_block = current_time_block;
        row.commits_count = 1;
      });
    } else {
      uint64_t new_random_number = compute_xor(random_numbers_iterator -> random_number, random_number);

      stats_index stats(get_self(), get_first_receiver().value);
      auto stats_iterator = stats.find(get_self().value);

      random_numbers.modify(random_numbers_iterator, _self, [&]( auto& row ) {
        row.random_number = new_random_number;
        row.commits_count += 1;

        int commits_count = random_numbers_iterator -> commits_count + 1;
        int oracles_count = stats_iterator -> oracles_count;

        row.valid = (float)commits_count / (float)oracles_count >= participation_requirement_threshold;
      });
    }
  };

  void remove_oracle_data(name user) {
    // First, removing oracle row.
    oracles_index oracles(get_self(), get_first_receiver().value);

    auto oracles_iterator = oracles.find(user.value);

    check(oracles_iterator != oracles.end(), "Oracle does not exist.");

    check(
      oracles_iterator -> last_miss_timestamp + 604800 < current_time_block(),
      "Last miss was less than a week ago."
    );

    oracles.erase(oracles_iterator);

    // Second, removing the committed number if it exists.
    committed_numbers_index committed_numbers(get_self(), get_first_receiver().value);

    auto committed_number_iterator = committed_numbers.find(user.value);

    if (committed_number_iterator != committed_numbers.end()) {
      committed_numbers.erase(committed_number_iterator);
    }

    // Third, updating the oracles count value.
    stats_index stats(get_self(), get_first_receiver().value);
    auto stats_iterator = stats.find(get_self().value);

    if (stats_iterator != stats.end()) {
      stats.modify(stats_iterator, _self, [&]( auto& row ) {
        row.oracles_count -= 1;
      });
    }
  };

  uint64_t compute_xor(uint64_t x, uint64_t y)
  {
    uint64_t res = 0;

    for (int i = 63; i >= 0; i--) {
       // Find current bits in x and y
      bool b1 = x & (1 << i);
      bool b2 = y & (1 << i);

      // If both are 1 then 0 else xor is same as OR
      bool xored_bit = (b1 & b2) ? 0 : (b1 | b2);

      res <<= 1;
      res |= xored_bit;
    }
    return res;
  }

  // Current time block number is defined as the number of seconds since UNIX epoch.
  uint32_t current_time_block() {
    return eosio::current_time_point().time_since_epoch().count() / 1000000;
  }

  // Tables declaration:
  struct [[eosio::table]] stats {
    name key;
    uint32_t oracles_count;

    uint64_t primary_key() const { return key.value; }
  };

  struct [[eosio::table]] oracles {
    name key;
    uint32_t reg_at_block_time;
    uint32_t misses_count;
    uint32_t last_miss_timestamp;

    uint64_t primary_key() const { return key.value; }
  };

  struct [[eosio::table]] committed_numbers {
    name key;
    eosio::checksum256 hash;
    uint32_t reveal_time_block;
    uint64_t revealed_number;

    uint64_t primary_key() const { return key.value; }
  };

  struct [[eosio::table]] random_numbers {
    uint32_t time_block;
    uint64_t random_number;
    uint32_t commits_count;
    bool valid;

    uint64_t primary_key() const {
      return static_cast<uint64_t>(time_block);
    }

    // Added a secondary index to allow for filtering by the valid field.
    uint64_t get_secondary_1() const {
      return static_cast<uint64_t>(valid);
    }
  };

  typedef eosio::multi_index<"stat"_n, stats> stats_index;
  typedef eosio::multi_index<"oracle"_n, oracles> oracles_index;
  typedef eosio::multi_index<"commitnumber"_n, committed_numbers> committed_numbers_index;
  typedef eosio::multi_index<"randomnumber"_n, random_numbers,
    indexed_by<"byvalid"_n, const_mem_fun<random_numbers, uint64_t, &random_numbers::get_secondary_1>>
    > random_numbers_index;
};
