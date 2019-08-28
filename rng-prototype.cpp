#include <eosio/eosio.hpp>
#include <eosio/print.hpp>

using namespace eosio;

class [[eosio::contract("greymass")]] greymass : public eosio::contract {

public:
  
  greymass(name receiver, name code,  datastream<const char*> ds): contract(receiver, code, ds) {}

  [[eosio::action]]
  void upsertnumber(name user, std::string hash, uint64_t block_number, std::string reveal) {
    require_auth(user);
    random_number_index random_numbers(get_first_receiver(), get_first_receiver().value);
    auto iterator = random_numbers.find(user.value);
    if( iterator == random_numbers.end() )
    {
      random_numbers.emplace(user, [&]( auto& row ) {
       row.key = user;
       row.hash = hash;
       row.block_number = block_number;
      });
    }
    else {
      random_numbers.modify(iterator, user, [&]( auto& row ) {
        row.reveal = reveal;
      });
    }
  }

  [[eosio::action]]
  void erase(name user) {
    require_auth(user);

    random_number_index random_numbers(get_first_receiver(), get_first_receiver().value);

    auto iterator = random_numbers.find(user.value);
    check(iterator != random_numbers.end(), "Record does not exist");
    random_numbers.erase(iterator);
    send_summary(user, " successfully erased record from random_numbers");
  }

  [[eosio::action]]
  void notify(name user, std::string msg) {
    require_auth(get_self());
    require_recipient(user);
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

  void send_summary(name user, std::string message) {
    action(
      permission_level{get_self(),"active"_n},
      get_self(),
      "notify"_n,
      std::make_tuple(user, name{user}.to_string() + message)
    ).send();
  };


  typedef eosio::multi_index<"hashes"_n, random_number, 
    indexed_by<"byblock"_n, const_mem_fun<random_number, uint64_t, &random_number::get_secondary_1>>
  > random_number_index;
  
};