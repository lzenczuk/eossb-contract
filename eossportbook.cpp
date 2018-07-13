#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/datastream.hpp>
#include <eosiolib/serialize.hpp>
#include <eosiolib/transaction.hpp>

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

    const uint8_t pending = 0;
    const uint8_t accepted = 1;
    const uint8_t rejected = 2;

    // @abi table offers i64
    struct offer {
        uint64_t offer_id;
        uint64_t runner_id;
        account_name originator;
        double price;
        int64_t amount;
        uint8_t status;

        uint64_t primary_key() const { return offer_id; }

        EOSLIB_SERIALIZE(offer, (offer_id)(runner_id)(originator)(price)(amount)(status))
    };

    typedef eosio::multi_index<N(offers), offer> offer_index;

    const uint8_t matched = 0;
    const uint8_t win = 1;
    const uint8_t lost = 2;
    const uint8_t payed = 3; // this for global bets table

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
        uint8_t status;

        uint64_t primary_key() const { return bet_id; }
        uint64_t mb_offer_index() const { return mb_offer_id; }

        EOSLIB_SERIALIZE(bet, (bet_id)(offer_id)(mb_offer_id)(runner_id)(originator)(acceptor)(price)(originator_amount)(acceptor_amount)(status))
    };

    typedef eosio::multi_index<N(bets), bet,
            eosio::indexed_by<N(betsbymbo), eosio::const_mem_fun<bet, uint64_t , &bet::mb_offer_index> >
    > bet_index;

    // @abi action
    void transfer(account_name sender, account_name receiver, eosio::asset amount, std::string &memo) {
        std::vector<std::string> params = parse_memo(memo);

        if (!params.empty()) {

            // submit offer
            // cleos transfer tester eossportbook "12 SYS" "so:289871:2500"
            // submit offer (so), runner id (289871), decimal ods * 1000 (2500)
            if (params.size() == 3 && params[0] == "so") {

                eosio::print("Submit offer.");

                uint64_t runner_id = std::strtoull(params[1].c_str(), NULL, 0);
                uint64_t price = std::strtoull(params[2].c_str(), NULL, 0);

                submit_offer(sender, runner_id, price / 1000.0, amount.amount);

            } else if (params.size() == 3 && params[0] == "ao") {

                // accept offer
                // cleos transfer tester eossportbook "12 SYS" "ao:21:349289"
                // accept offer (ao), offer id in contract (21), opposite matchbook offer id (349289)

                eosio::print("Accept offer.");

                uint64_t offer_id = std::strtoull(params[1].c_str(), NULL, 0);
                uint64_t mb_offer_id = std::strtoull(params[2].c_str(), NULL, 0);

                accept_offer(sender, offer_id, mb_offer_id, amount.amount);

            }

        }
    }

    std::vector<std::string> parse_memo(std::string memo) {
        vector<string> tokens;

        std::size_t pos = 0;
        while ((pos = memo.find(":")) != string::npos) {
            tokens.push_back(memo.substr(0, pos));
            memo.erase(0, pos + 1); // +1 for delimiter (:)
        }

        if (!memo.empty()) {
            tokens.push_back(memo);
        }

        return tokens;
    }

    void submit_offer(account_name originator, uint64_t runner_id, double price, int64_t amount) {

        uint64_t new_offer_id = get_id();

        // Table with all offers in contract scope
        offer_index global_offers_db(_self, _self);
        global_offers_db.emplace(_self, [&](offer &nof) {
            nof.offer_id = new_offer_id;
            nof.runner_id = runner_id;
            nof.originator = originator;
            nof.price = price;
            nof.amount = amount;
            nof.status = pending;
        });

        // Table with account's offer in account scope
        offer_index originator_offers_db(_self, originator);
        originator_offers_db.emplace(_self, [&](offer &nof) {
            nof.offer_id = new_offer_id;
            nof.runner_id = runner_id;
            nof.originator = originator;
            nof.price = price;
            nof.amount = amount;
            nof.status = pending;
        });
    }

    void accept_offer(account_name acceptor, uint64_t offer_id, uint64_t mb_offer_id, int64_t amount) {

        uint64_t new_bet_id = get_id();

        offer_index global_offers_db(_self, _self);
        auto itr = global_offers_db.find(offer_id);
        if(itr != global_offers_db.end()){
            offer of = *itr;

            eosio::print("-------------> insert global bet \n");

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
                nb.status = matched;
            });

            eosio::print("-------------> insert originator bet \n");

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
                nb.status = matched;
            });

            eosio::print("-------------> insert acceptor bet \n");

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
                nb.status = matched;
            });

            eosio::print("-------------> update global offer \n");

            global_offers_db.modify(itr, 0, [&](offer &of2u) {
                of2u.status = accepted;
            });

            eosio::print("-------------> update originator offer \n");

            // Update offer status in originator scope
            offer_index originator_offers_db(_self, of.originator);
            auto ofitr = originator_offers_db.find(of.offer_id);
            if(ofitr != originator_offers_db.end()){
                originator_offers_db.modify(ofitr, 0, [&](offer &of2u) {
                    of2u.status = accepted;
                });
            }
        }
    }

    const uint8_t originator_win = 0;
    const uint8_t acceptor_win = 1;

    // @abi action
    void payout(uint64_t mb_offer_id, uint8_t winner){

        bet_index global_bets_db(_self, _self);
        auto mbo_bets_db = global_bets_db.get_index<N(betsbymbo)>();

        auto itr = mbo_bets_db.find(mb_offer_id);
        if(itr != mbo_bets_db.end()){

            bet b = *itr;

                // send all funds to acceptor
                eosio::print("-----------> create action\n");
            /*eosio::action transfer_action = eosio::action(
                                    eosio::permission_level{ _self, N(active) },
                                    N(eosio.token), N(transfer),
                                    std::make_tuple(_self, b.acceptor, eosio::asset(b.originator_amount+b.acceptor_amount, S(4, EOS)), std::string("Win bet"))
                            );

            eosio::print("-----------> create transaction\n");
            eosio::transaction ft = eosio::transaction();*/

            eosio::transaction out;
            out.actions.emplace_back(eosio::permission_level{_self, N(active)}, N(eosio.token), N(transfer), std::make_tuple(_self, b.acceptor, eosio::asset(b.originator_amount+b.acceptor_amount, S(4, SYS)), std::string("Win bet")));
            out.delay_sec = 5;
            out.send(get_id(), _self);

            /*eosio::print("-----------> add action to transaction\n");
                //ft.actions.push_back(transfer_action);
                ft.actions.push_back(transfer_action);

                //const eosio::bytes &serialized_transfer_action = pack(transfer_action);

                eosio::print("-----------> send transaction\n");
                //send_deferred(1236534, _self, serialized_transfer_action.data(), serialized_transfer_action.size());
                ft.send(123, _self);*/
                eosio::print("-----------> done\n");
                //transfer_action.send();


            // update global bet status
            auto gbitr = global_bets_db.find(b.bet_id);
            if(gbitr != global_bets_db.end()){
                global_bets_db.modify(gbitr, 0, [&](bet &b2u) {
                    b2u.status = payed;
                });
            }

            // update originator bet status
            bet_index originator_bets_db(_self, b.originator);
            auto obitr = originator_bets_db.find(b.bet_id);
            if(obitr != originator_bets_db.end()){
                originator_bets_db.modify(obitr, 0, [&](bet &b2u) {
                    if(winner == originator_win){
                        b2u.status = win;
                    }else{
                        b2u.status = lost;
                    }
                });
            }

            // update acceptor bet status
            bet_index acceptor_bets_db(_self, b.acceptor);
            auto abitr = acceptor_bets_db.find(b.bet_id);
            if(abitr != acceptor_bets_db.end()){
                acceptor_bets_db.modify(abitr, 0, [&](bet &b2u) {
                    if(winner == originator_win){
                        b2u.status = lost;
                    }else{
                        b2u.status = win;
                    }
                });
            }
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

EOSIO_ABI_EX(eossportbook, (transfer)(payout))
