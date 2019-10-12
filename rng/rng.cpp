#include <eosio/eosio.hpp>
#include <eosio/print.hpp>
#include <eosio/system.hpp>
#include <eosio/crypto.hpp>

using namespace eosio;

class [[eosio::contract("rng")]] rng : public eosio::contract {

public:

  rng(name receiver, name code,  datastream<const char*> ds): contract(receiver, code, ds) {}

  [[eosio::action]]
  void commitnumber(name user, eosio::checksum256 hash, uint32_t reveal_time_block) {
    require_auth(user);

    committed_numbers_index committed_numbers(get_first_receiver(), get_first_receiver().value);

    auto iterator = committed_numbers.find(user.value);

    if (iterator == committed_numbers.end()) {
      // Make sure that we are not committing to the current block.
      if (reveal_time_block == current_time_block()) {
        return print("Cannot commit to revealing a number during current block.");
      }

      committed_numbers.emplace(user, [&]( auto& row ) {
        row.key = user;
        row.hash = hash;
        row.reveal_time_block = reveal_time_block;
      });
    } else {
      committed_numbers.modify(iterator, user, [&]( auto& row ) {
        row.hash = hash;
        row.reveal_time_block = reveal_time_block;
        row.revealed_number = NULL;
      });
    }
  }

  [[eosio::action]]
  void revealnumber(name user, uint32_t reveal_time_block, uint32_t revealed_number) {
    require_auth(user);

    committed_numbers_index committed_numbers(get_first_receiver(), get_first_receiver().value);

    auto iterator = committed_numbers.find(user.value);

    // If already revealed, return an error
    if (iterator -> revealed_number != 0) {
      return print("Number has already been revealed!");
    }

    // If not at correct reveal time, return an error.
    if (current_time_block() != iterator -> reveal_time_block) {
      return print("Commit has expired!");
    }

    std::string revealed_number_string = std::to_string(revealed_number);
    char const *revealed_number_char = revealed_number_string.c_str();

    // will raise an error if invalid hash
    assert_sha256(revealed_number_char, revealed_number_string.length(), iterator -> hash);

    committed_numbers.modify(iterator, user, [&]( auto& row ) {
      row.revealed_number = revealed_number;
      row.previous_time_block = iterator -> reveal_time_block;
    });

    update_global_random_number(user, revealed_number, iterator -> reveal_time_block);
  }

  [[eosio::action]]
  void erase(name user) {
    require_auth(user);

    committed_numbers_index committed_numbers(get_first_receiver(), get_first_receiver().value);

    auto iterator = committed_numbers.find(user.value);
    check(iterator != committed_numbers.end(), "Record does not exist");
    committed_numbers.erase(iterator);
  }

private:
  struct [[eosio::table]] committed_numbers {
    name key;
    eosio::checksum256 hash;
    uint32_t reveal_time_block;
    uint32_t revealed_number;
    uint32_t now_time_block;
    uint32_t previous_time_block;


    uint64_t primary_key() const { return key.value; }
  };

  struct [[eosio::table]] random_numbers {
    uint64_t time_block;
    uint32_t random_number;
    uint32_t commits_count;

    uint64_t primary_key() const { return time_block; }
  };

  void update_global_random_number(name user, int random_number, int current_time_block) {
    random_numbers_index random_numbers(get_first_receiver(), get_first_receiver().value);

    auto iterator = random_numbers.find(current_time_block);

    if (iterator == random_numbers.end()) {
      random_numbers.emplace(user, [&]( auto& row ) {
        row.random_number = random_number;
        row.time_block = current_time_block;
        row.commits_count = 1;
      });
    } else {
      uint64_t new_random_number = compute_xor(iterator -> random_number, random_number);

      random_numbers.modify(iterator, user, [&]( auto& row ) {
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

  typedef eosio::multi_index<"committednum"_n, committed_numbers> committed_numbers_index;
  typedef eosio::multi_index<"randomnumber"_n, random_numbers> random_numbers_index;
};
