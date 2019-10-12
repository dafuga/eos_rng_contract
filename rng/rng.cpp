#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include <eosio/crypto.hpp>

//#include <eosio/print.hpp>

using namespace eosio;

class [[eosio::contract("rng")]] rng : public eosio::contract {
  int oracle_registration_delay = 1000;
  int participation_requirement_threshold = 2;

public:
  rng(name receiver, name code, datastream<const char*> ds): contract(receiver, code, ds) {}

  [[eosio::action]]
  void commitnumber(name user, eosio::checksum256 hash, uint32_t reveal_time_block) {
    require_auth(user);

    uint32_t current_block = current_time_block();

    oracles_index oracles(get_first_receiver(), get_first_receiver().value);
    auto oracles_iterator = oracles.find(user.value);

    bool is_invalid_oracle =
      oracles_iterator == oracles.end() ||
      current_block - oracle_registration_delay < oracles_iterator -> reg_at_block_time;

    check(
      is_invalid_oracle,
      "Commits can only be made by oracles that have been registered for 1000 time blocks."
    );

    committed_numbers_index committed_numbers(get_first_receiver(), get_first_receiver().value);
    auto committed_numbers_iterator = committed_numbers.find(user.value);

    if (committed_numbers_iterator -> revealed_number == NULL) {
      remove_oracle_data(user);
    }

    if (committed_numbers_iterator == committed_numbers.end()) {
      // Make sure that we are not committing to the current block.
      check(
        reveal_time_block == current_block,
        "Cannot commit to revealing a number during current block."
      );

      committed_numbers.emplace(user, [&]( auto& row ) {
        row.key = user;
        row.hash = hash;
        row.reveal_time_block = reveal_time_block;
      });
    } else {
      committed_numbers.modify(committed_numbers_iterator, user, [&]( auto& row ) {
        row.hash = hash;
        row.reveal_time_block = reveal_time_block;
      });
    }
  }

  [[eosio::action]]
  void revealnumber(name user, uint32_t reveal_time_block, uint32_t revealed_number) {
    require_auth(user);

    committed_numbers_index committed_numbers(get_first_receiver(), get_first_receiver().value);

    auto iterator = committed_numbers.find(user.value);

    // If already revealed, return an error.
    check(iterator -> revealed_number != NULL, "Number has already been revealed!");

    // If not at correct reveal time, return an error.
    check(current_time_block() != iterator -> reveal_time_block, "Commit has expired!");

    std::string revealed_number_string = std::to_string(revealed_number);
    char const *revealed_number_char = revealed_number_string.c_str();

    // will raise an error if invalid hash
    assert_sha256(revealed_number_char, revealed_number_string.length(), iterator -> hash);

    committed_numbers.modify(iterator, user, [&]( auto& row ) {
      row.revealed_number = revealed_number;
    });

    update_global_random_number(user, revealed_number, iterator -> reveal_time_block);
  }

  [[eosio::action]]
  void regoracle(name user) {
    require_auth(user);

    oracles_index oracles(get_first_receiver(), get_first_receiver().value);

    oracles.emplace(_self, [&]( auto& row ) {
      row.key = user;
      row.reg_at_block_time = current_time_block();
    });
  }

  [[eosio::action]]
  void unregoracle(name user) {
    require_auth(user);

    remove_oracle_data(user);
  }

  [[eosio::action]]
  void cleannumbers() {
    require_auth(_self);

    uint32_t time_block = current_time_block();

    random_numbers_index random_numbers(get_first_receiver(), get_first_receiver().value);

    for(auto& random_number : random_numbers) {
      if (random_number.time_block < time_block - 100) {
        auto iterator = random_numbers.find(random_number.time_block);

        random_numbers.erase(iterator);
      }
    }
  }

private:
  struct [[eosio::table]] oracles {
    name key;
    uint32_t reg_at_block_time;

    uint64_t primary_key() const { return key.value; }
  };

  struct [[eosio::table]] committed_numbers {
    name key;
    eosio::checksum256 hash;
    uint32_t reveal_time_block;
    std::optional<uint32_t> revealed_number;

    uint64_t primary_key() const { return key.value; }
  };

  struct [[eosio::table]] random_numbers {
    uint32_t time_block;
    uint32_t random_number;
    uint32_t commits_count;

    uint64_t primary_key() const {
      uint64_t int64_time_block = time_block;

      return int64_time_block;
    }
  };

  void remove_oracle_data(name user) {
    oracles_index oracles(get_first_receiver(), get_first_receiver().value);

    auto oracles_iterator = oracles.find(user.value);

    check(oracles_iterator != oracles.end(), "Oracle does not exist");

    oracles.erase(oracles_iterator);

    committed_numbers_index committed_numbers(get_first_receiver(), get_first_receiver().value);

    auto committed_number_iterator = committed_numbers.find(user.value);

    if (committed_number_iterator != committed_numbers.end()){
      committed_numbers.erase(committed_number_iterator);
    }
  }

  void update_global_random_number(name user, int random_number, int current_time_block) {
    random_numbers_index random_numbers(get_first_receiver(), get_first_receiver().value);

    auto iterator = random_numbers.find(current_time_block);

    if (iterator == random_numbers.end()) {
      random_numbers.emplace(_self, [&]( auto& row ) {
        row.random_number = random_number;
        row.time_block = current_time_block;
        row.commits_count = 1;
      });
    } else {
      uint64_t new_random_number = compute_xor(iterator -> random_number, random_number);

      random_numbers.modify(iterator, _self, [&]( auto& row ) {
        row.random_number = new_random_number;
        row.commits_count += 1;
      });
    }
  };

  uint64_t compute_xor(uint64_t x, uint64_t y)
  {
    uint64_t res = 0;

    for (int i = 31; i >= 0; i--) {
       // Find current bits in x and y
      bool b1 = x & (1 << i);
      bool b2 = y & (1 << i);

      // If both are 1 then 0 else xor is same as OR
      bool xoredBit = (b1 & b2) ? 0 : (b1 | b2);

      res <<= 1;
      res |= xoredBit;
    }
    return res;
  }

  uint32_t current_time_block() {
    return eosio::current_time_point().time_since_epoch().count() / 1000000;
  }

  typedef eosio::multi_index<"oracle"_n, oracles> oracles_index;
  typedef eosio::multi_index<"committednum"_n, committed_numbers> committed_numbers_index;
  typedef eosio::multi_index<"randomnumber"_n, random_numbers> random_numbers_index;
};
