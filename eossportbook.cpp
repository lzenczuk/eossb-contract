#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/serialize.hpp>

class eossportbook : public eosio::contract {
public:
    eossportbook(account_name self) : eosio::contract(self) {}

    // @abi table identry i64
    struct identry {
        uint64_t key;
        uint64_t last_value;

        uint64_t primary_key() const { return key; }

        EOSLIB_SERIALIZE(identry, (key)(last_value))
    };

    typedef eosio::multi_index<N(identries), identry> identy_index;

    uint64_t get_id(){
        identy_index id_entry_db(_self, _self);

        auto itr = id_entry_db.find(N(offere_key));
        if(itr == id_entry_db.end()){
            id_entry_db.emplace(_self, [&](auto &ide) {
                ide.key = N(offere_key);
                ide.last_value = 0;
            });

            return 0;
        }else{

            identry i = *itr;
            uint64_t new_value = i.last_value+1;

            id_entry_db.modify( itr, 0, [&]( auto& ide ) {
                ide.last_value = new_value;
            });

            return new_value;
        }
    }

    // @abi table offers i64
    struct offer {
        uint64_t offer_id;
        uint64_t runner_id;
        account_name originator;
        double price;
        int64_t amount;

        uint64_t primary_key() const { return offer_id; }

        EOSLIB_SERIALIZE(offer, (offer_id)(runner_id)(originator)(price)(amount))
    };

    typedef eosio::multi_index<N(offers), offer> offer_index;

    // @abi action
    void transfer(account_name sender, account_name receiver, eosio::asset amount, std::string& memo){
        eosio::print("=====> Transfer: ", sender, " -> ", receiver, "; amount: ", amount, "; info: ", memo);

        if(receiver==N(eossportbook)){
            eosio::print("This is my transfer. Parse and create offer or bet");
        }else{
            eosio::print("#NotMyTransaction");
        }
    }

    // @abi action
    void submitoffer(account_name originator, uint64_t runner_id, double price, int64_t amount){
        require_auth(originator);

        eosio::print("------> submitoffer ", originator, "; ", runner_id, "; ", price, "; ", amount, "\n");

        eosio_assert(price > 0.0, "offer price have to be bigger then 0");
        eosio_assert(amount > 0.0, "offer amount have to be bigger then 0");

        eosio::print("------> getting new offer id\n");

        uint64_t new_offer_id = get_id();

        eosio::print("------> new offer id", new_offer_id, "\n");

        eosio::print("------> inserting offer to global scope\n");
        // Table with all offers in contract scope
        offer_index global_offers_db(_self, _self);
        global_offers_db.emplace(_self, [&](offer &nof) {
            nof.offer_id = new_offer_id;
            nof.runner_id = runner_id;
            nof.originator = originator;
            nof.price = price;
            nof.amount = amount;
        });

        eosio::print("------> inserting offer to originator scope\n");
        // Table with account's offer in account scope
        offer_index originator_offers_db(_self, originator);
        originator_offers_db.emplace(_self, [&](offer &nof) {
            nof.offer_id = new_offer_id;
            nof.runner_id = runner_id;
            nof.originator = originator;
            nof.price = price;
            nof.amount = amount;
        });

        // ERROR - this won't work!!!!!
        eosio::print("------> transfering funds\n");
        eosio::action(
                eosio::permission_level { originator,N(active)},
                N(eosio.token),N(transfer),
                std::make_tuple(originator, _self, eosio::asset(amount, CORE_SYMBOL),"Submit offer")
        ).send();
    }

};

#define EOSIO_ABI_EX( TYPE, MEMBERS ) \
extern "C" { \
   void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
      if( action == N(onerror)) { \
         /* onerror is only valid if it is for the "eosio" code account and authorized by "eosio"'s "active permission */ \
         eosio_assert(code == N(eosio), "onerror action's are only valid from the \"eosio\" system account"); \
      } \
      auto self = receiver; \
      if( code == self || code == N(eosio.token) || action == N(onerror) ) { \
         TYPE thiscontract( self ); \
         switch( action ) { \
            EOSIO_API( TYPE, MEMBERS ) \
         } \
         /* does not allow destructor of thiscontract to run: eosio_exit(0); */ \
      } \
   } \
}

EOSIO_ABI_EX(eossportbook, (submitoffer)(transfer))
