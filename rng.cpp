#include <eosio/eosio.hpp>
#include <eosio/print.hpp>

using namespace eosio;

class [[eosio::contract("random_number_generator")]] random_number_generator : public eosio::contract {

public:
  
  random_number_generator(name receiver, name code,  datastream<const char*> ds): contract(receiver, code, ds) {}

  [[eosio::action]]
  void upsert_random_number_entry(name user, std::string initial_hash, uint64_t block_number, std::string reveal_hash) {
    require_auth(user);
    random_number_index random_numbers(get_first_receiver(), get_first_receiver().value);
    auto iterator = random_numbers.find(user.value);
    if( iterator == random_numbers.end() )
    {
      random_numbers.emplace(user, [&]( auto& row ) {
       row.name = user;
       row.initial_hash = reveal_hash;
       row.block_number = block_number;
      });
      send_summary(user, " successfully emplaced record to random_numbers");
    }
    else {
      random_numbers.modify(iterator, user, [&]( auto& row ) {
        row.reveal_string = reveal_string;
      });
      send_summary(user, " successfully modified record to random_numbers");
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
    std::string initial_hash;
    uint64_t block_number;
    std::string reveal_hash;
  
    uint64_t primary_key() const { return key.value; }
    uint64_t get_secondary_1() const { return age;}
  };

  void send_summary(name user, std::string message) {
    action(
      permission_level{get_self(),"active"_n},
      get_self(),
      "notify"_n,
      std::make_tuple(user, name{user}.to_string() + message)
    ).send();
  };


  typedef eosio::multi_index<"people"_n, person, 
    indexed_by<"byage"_n, const_mem_fun<person, uint64_t, &person::get_secondary_1>>
  > random_number_index;
  
};