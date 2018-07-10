#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/serialize.hpp>

using std::string;
using std::vector;

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

    typedef eosio::multi_index<N(identry), identry> identy_index;

    uint64_t get_id() {
        identy_index id_entry_db(_self, _self);

        auto itr = id_entry_db.find(N(offere_key));
        if (itr == id_entry_db.end()) {
            id_entry_db.emplace(_self, [&](auto &ide) {
                ide.key = N(offere_key);
                ide.last_value = 0;
            });

            return 0;
        } else {

            identry i = *itr;
            uint64_t new_value = i.last_value + 1;

            id_entry_db.modify(itr, 0, [&](auto &ide) {
                ide.last_value = new_value;
            });

            return new_value;
        }
    }

    // @abi table logs i64
    struct log {
        uint64_t log_id;
        string msg;

        uint64_t primary_key() const { return log_id; }

        EOSLIB_SERIALIZE(log, (log_id)(msg))
    };

    typedef eosio::multi_index<N(logs), log> log_index;

    void add_log(const char *m) {
        log_index log_db(_self, _self);

        uint64_t log_id = get_id();
        string msg(m);

        log_db.emplace(_self, [&](log &l) {
            l.log_id = log_id;
            l.msg = msg;
        });
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

    // @abi table bets i64
    struct bet {
        uint64_t bet_id;
        uint64_t offer_id;
        uint64_t mb_offer_id;
        uint64_t runner_id;
        account_name originator;
        account_name acceptor;
        double price;
        int64_t originator_amount;
        int64_t acceptor_amount;

        uint64_t primary_key() const { return bet_id; }

        EOSLIB_SERIALIZE(bet, (bet_id)(offer_id)(mb_offer_id)(runner_id)(originator)(acceptor)(price)(originator_amount)(acceptor_amount))
    };

    typedef eosio::multi_index<N(bets), bet> bet_index;

    // @abi action
    void inc() {
        get_id();
        add_log("Inc log test");
    }

    // @abi action
    void transfer(account_name sender, account_name receiver, eosio::asset amount, std::string &memo) {
        eosio::print("=====> Transfer: ", sender, " -> ", receiver, "; amount: ", amount, "; info: ", memo);

        add_log("Receive transfer");

        add_log("processing");
        eosio::print("This is my transfer. Parse and create offer or bet");

        std::vector<std::string> params = parse_memo(memo);

        if (params.empty()) {
            eosio::print("Params are empty.");
        } else {

            eosio::print("Params are not empty. ", params.size());

            // submit offer
            // cleos transfer tester eossportbook "12 SYS" "so:289871:2500"
            if (params.size() == 3 && params[0] == "so") {

                eosio::print("Submit offer.");

                uint64_t runner_id = std::strtoull(params[1].c_str(), NULL, 0);
                uint64_t price = std::strtoull(params[2].c_str(), NULL, 0);

                submit_offer(sender, runner_id, price / 1000.0, amount.amount);

            } else if (params.size() == 3 && params[0] == "ao") {

                eosio::print("Accept offer.");

                uint64_t offer_id = std::strtoull(params[1].c_str(), NULL, 0);
                uint64_t mb_offer_id = std::strtoull(params[2].c_str(), NULL, 0);

                accept_offer(sender, offer_id, mb_offer_id, amount.amount);

            } else {
                eosio::print("Unknown operation.");
            }

        }
    }

    std::vector<std::string> parse_memo(std::string memo) {
        vector<string> tokens;

        std::size_t pos = 0;
        while ((pos = memo.find(":")) != string::npos) {
            eosio::print("Substring: ", memo.substr(0, pos).c_str());
            tokens.push_back(memo.substr(0, pos));
            memo.erase(0, pos + 1); // +1 for delimiter (:)
            eosio::print("Memo after erase: ", memo);
        }

        if (!memo.empty()) {
            tokens.push_back(memo);
        }

        return tokens;
    }

    void submit_offer(account_name originator, uint64_t runner_id, double price, int64_t amount) {

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
    }

    void accept_offer(account_name acceptor, uint64_t offer_id, uint64_t mb_offer_id, int64_t amount) {

        eosio::print("------> accept ", acceptor, "; ", offer_id, "; ", mb_offer_id);

        uint64_t new_bet_id = get_id();

        eosio::print("------> new bet id", new_bet_id, "\n");

        offer_index global_offers_db(_self, _self);
        auto itr = global_offers_db.find(offer_id);
        if(itr == global_offers_db.end()){
            add_log("Offer not found");
            eosio::print("------> offer not found", offer_id, "\n");
        }else{
            offer of = *itr;

            add_log("Add global bet");
            bet_index global_bets_db(_self, _self);
            global_bets_db.emplace(_self, [&](bet& nb) {
                nb.bet_id = new_bet_id;
                nb.offer_id = of.offer_id;
                nb.mb_offer_id = mb_offer_id;
                nb.runner_id = of.runner_id;
                nb.price = of.price;
                nb.originator = of.originator;
                nb.originator_amount = of.amount;
                nb.acceptor = acceptor;
                nb.acceptor_amount = amount;
            });

            add_log("Add originator bet");
            bet_index originator_bets_db(_self, of.originator);
            originator_bets_db.emplace(_self, [&](bet& nb) {
                nb.bet_id = new_bet_id;
                nb.offer_id = of.offer_id;
                nb.mb_offer_id = mb_offer_id;
                nb.runner_id = of.runner_id;
                nb.price = of.price;
                nb.originator = of.originator;
                nb.originator_amount = of.amount;
                nb.acceptor = acceptor;
                nb.acceptor_amount = amount;
            });

            add_log("Add acceptor bet");
            bet_index acceptor_bets_db(_self, acceptor);
            acceptor_bets_db.emplace(_self, [&](bet& nb) {
                nb.bet_id = new_bet_id;
                nb.offer_id = of.offer_id;
                nb.mb_offer_id = mb_offer_id;
                nb.runner_id = of.runner_id;
                nb.price = of.price;
                nb.originator = of.originator;
                nb.originator_amount = of.amount;
                nb.acceptor = acceptor;
                nb.acceptor_amount = amount;
            });
        }
    }

};

#define EOSIO_ABI_EX(TYPE, MEMBERS) \
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

EOSIO_ABI_EX(eossportbook, (transfer)(inc))
