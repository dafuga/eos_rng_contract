#include <eosio/eosio.hpp>
#include <eosio/print.hpp>
#include <eosio/system.hpp>
#include <eosio/crypto.hpp>

using namespace eosio;

class [[eosio::contract("greymass")]] greymass : public eosio::contract {

public:
  
  greymass(name receiver, name code,  datastream<const char*> ds): contract(receiver, code, ds) {}

  [[eosio::action]]
  void upsertnumber(name user, eosio::checksum256 hash, uint32_t revealed_number) {
    require_auth(user);
    committed_r_number_index committed_r_numbers(get_first_receiver(), get_first_receiver().value);
    auto iterator = committed_r_numbers.find(user.value);
    if( iterator == committed_r_numbers.end() )
    {
      committed_r_numbers.emplace(user, [&]( auto& row ) {
       row.key = user;
       row.hash = hash;
       row.reveal_time = (eosio::current_time_point().sec_since_epoch() + 10);
      });
    } else {
      bool validRevealTime =
        (eosio::current_time_point().sec_since_epoch() > iterator -> reveal_time &&
        eosio::current_time_point().sec_since_epoch() < iterator -> reveal_time + 10);

      if (!validRevealTime) {
        return;
      }

      std::string revealed_number_string = std::to_string(iterator -> revealed_number);
      char const *revealed_number_char = revealed_number_string.c_str();

      // will raise an error if invalid hash
      assert_sha256(revealed_number_char, 3, iterator -> hash );

      committed_r_numbers.modify(iterator, user, [&]( auto& row ) {
        row.revealed_number = revealed_number;
      });

      int closest_block = iterator -> reveal_time / 500;

      modify_global_random_number(user, revealed_number, closest_block);
    }
  }

  [[eosio::action]]
  void erase(name user) {
    require_auth(user);

    committed_r_number_index committed_r_numbers(get_first_receiver(), get_first_receiver().value);

    auto iterator = committed_r_numbers.find(user.value);
    check(iterator != committed_r_numbers.end(), "Record does not exist");
    committed_r_numbers.erase(iterator);
  }

private:
  struct [[eosio::table]] committed_r_numbers {
    name key;
    eosio::checksum256 hash;
    uint64_t reveal_time;
    uint32_t revealed_number;

    uint64_t primary_key() const { return key.value; }
    uint64_t get_reveal_time() const { return reveal_time; }
  };

  struct [[eosio::table]] shared_r_numbers {
    uint64_t time_unit;
    uint64_t closest_block;
    uint32_t random_number;
  
    uint64_t primary_key() const { return time_unit; }
    uint64_t get_random_number() const { return random_number; }
  };

  void modify_global_random_number(name user, int random_number, int closest_block) {
    shared_r_number_index shared_r_numbers(get_first_receiver(), get_first_receiver().value);

    auto iterator = shared_r_numbers.find(user.value);

    if (iterator != shared_r_numbers.end()) {
      shared_r_numbers.emplace(user, [&]( auto& row ) {
        row.random_number = random_number;
        row.closest_block = closest_block;
      });
    } else {
      uint64_t new_random_number = computeXOR(iterator -> random_number, random_number);

      shared_r_numbers.modify(iterator, _self, [&]( auto& row ) {
        row.random_number = new_random_number;
      });
    }
  };

  uint64_t computeXOR(uint64_t x, uint64_t y) 
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

  typedef eosio::multi_index<"cnumbers"_n, committed_r_numbers> committed_r_number_index;
  typedef eosio::multi_index<"snumbers"_n, shared_r_numbers> shared_r_number_index;
};
