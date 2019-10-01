#include <eosio/eosio.hpp>
#include <eosio/print.hpp>
#include <eosio/system.hpp>
#include <eosio/crypto.hpp>

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

      // will raise an error if invalid hash
      assert_sha256(iterator -> reveal, 3, iterator -> hash );

      rnumbers.modify(iterator, user, [&]( auto& row ) {
        row.reveal = reveal;
      });

      modify_global_random_number(user, revealed_number)
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
    eosio::checksum256 hash;
    uint64_t block_number;
    std::string reveal;

    uint64_t primary_key() const { return key.value; }
    uint64_t get_secondary_1() const { return block_number;}
  };


  struct [[eosio::table]] global_random_number {
    uint64_t time_unit;
    uint64_t global_random_value;
  
    uint64_t primary_key() const { return time_unit; }
    uint64_t get_random_number() const { return global_random_value; }
  };


  void modify_global_random_number(random_number) {
    gnumber_index rnumbers(get_first_receiver(), get_first_receiver().value);

    auto iterator = gnumbers.find(user.value);

    uint64_t new_random_number = computeXOR(iterator -> reveal_time);

    gnumbers.modify(iterator, self, [&]( auto& row ) {
      row.random_number = new_random_number;
    });
  };


  typedef eosio::multi_index<"rnumbers"_n, rnumber> rnumber_index;
  typedef eosio::multi_index<"gnumbers"_n, gnumber> gnumber_index;
};

uint64_t computeXOR(uint64_t x, uint64_t y) 
{ 
    uint64_t res = 0;
      
    for (int i = 31; i >= 0; i--)                      
    { 
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
