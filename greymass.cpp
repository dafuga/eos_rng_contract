#include <eosio/eosio.hpp>
#include <eosio/print.hpp>
#include <eosio/system.hpp>
#include "crypto_sign.h"

using namespace eosio;

class [[eosio::contract("greymass")]] greymass : public eosio::contract {

public:
  
  greymass(name receiver, name code,  datastream<const char*> ds): contract(receiver, code, ds) {}

  [[eosio::action]]
  void upsertnumber(name user, std::string hash, std::string revealed_number) {
    require_auth(user);
    rnumber_index rnumbers(get_first_receiver(), get_first_receiver().value);
    auto iterator = rnumbers.find(user.value);
    if( iterator == rnumbers.end() )
    {
      rnumbers.emplace(user, [&]( auto& row ) {
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

      bool validHash = crypto_sign_open(iterator -> hash, iterator -> reveal);

      if (!validHash) {
        return;
      }

      rnumbers.modify(iterator, user, [&]( auto& row ) {
        row.reveal = reveal;
      });

      modify_global_random_number(revealed_number)
    }
  }

  [[eosio::action]]
  void erase(name user) {
    require_auth(user);

    rnumber_index rnumbers(get_first_receiver(), get_first_receiver().value);

    auto iterator = rnumbers.find(user.value);
    check(iterator != rnumbers.end(), "Record does not exist");
    rnumbers.erase(iterator);
  }

private:
  struct [[eosio::table]] random_number {
    name key;
    std::string hash;
    uint64_t block_number;
    std::string reveal;

    uint64_t primary_key() const { return key.value; }
    uint64_t get_secondary_1() const { return block_number;}
  };


  struct [[eosio::table]] global_random_number {
    uint64_t time_unit;
    uint64_t random_number;
  
    uint64_t primary_key() const { return time_unit; }
    uint64_t get_random_number() const { return random_number; }
  };


  void modify_global_random_number(random_number) {
    auto iterator = gnumbers.find(user.value);

    uint64_t new_random_number = (iterator -> reveal_time) XOR random_number


    gnumbers.modify(iterator, self, [&]( auto& row ) {
      row.random_number = new_random_number;
    });
  };


  typedef eosio::multi_index<"rnumbers"_n, rnumber> rnumber_index;
    typedef eosio::multi_index<"gnumbers"_n, gnumber> gnumber_index;

  
};
