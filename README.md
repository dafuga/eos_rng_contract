## Random Number Generator
A Random Number Generator EOSIO Smart Contract that allows any number of oracles to contribute numbers that are used to generate a truly random number available on an EOSIO table.

The process works like such:
1) An oracle starts off by committing a hash of the number that they wish to contribute. As they contribute that hash, they will also specify the time block at which they will reveal the number. The time_block is an integer defined by the number of 500 ms trenches time that have passed since the Unix epoch in UTC.

    For example, assuming that Bob wants to commit the number `123`, he would first commit the following transaction:
    ```
    cleos push action greymass commitnumber '["bob", "a665a45920422f9d417e4867efdc4fb8a04a1f3fff1fa07e998e86f7f7a27ae3", 3140484404, null]' -p bob@active
    ```
    
2) Once the current time in UTC belongs to the time block defined in #1, the number can be revealed by passing it as an argument.
    In the case of Bob, that would look like this:
    ```
    cleos push action greymass commitnumber '["bob", "a665a45920422f9d417e4867efdc4fb8a04a1f3fff1fa07e998e86f7f7a27ae3", 3140484404, 123]' -p bob@active
    ```
    The smart contract will automatically validate that the number corresponds to the previously committed hash and that the current block time is also the one that was previously committed. If those conditions are met, then the number will be used in a XOR function to modify the number that is stored on the `snumbers` table for the given time_block. This means that the number becomes truly random as soon as two independent oracles contribute to this smart contract.
    
3) As soon as two oracles commits a number for a given block, the `snumbers` table can be used to get a randomly generated number at a given time_block.

    Eg.
    ```
    cleos get table greymass greymass snumbers --lower alice --limit 1
    ```

**Note:**
Steps 1 and 2 can be repeated at whatever interval you wish.
At any time, the entry can be deleted with the erase function which in the case of our friend Bob would be done like this:
    
Eg.
```
cleos push action greymass erase '["bob"]' -p bob@active
```
    

#### Get an Oracle process running.

There is a node process that can be used to ge
1) Run `cp .env.sample .env` and change the placeholders for the credentials of the account that you wish to use.

2) Run `npm install`;
3) Start the oracle process with
    ```
    node scripts/oracle.js <interval_in_seconds>
    ```
    Where "<interval_in_seconds>" is the interval of time between each random number commits defined in seconds.
