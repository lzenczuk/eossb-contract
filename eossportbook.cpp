#include <eosiolib/eosio.hpp>
#include <eosiolib/serialize.hpp>

class eossportbook : public eosio::contract {
public:
    eossportbook(account_name self) : eosio::contract(self),
                                      events(_self, _self),
                                      runners(_self, _self),
                                      offers(_self, _self) {}

    // @abi table event i64
    struct event {
        uint64_t event_id;
        std::string event_name;

        uint64_t primary_key() const { return event_id; }

        EOSLIB_SERIALIZE(event, (event_id)(event_name))
    };

    typedef eosio::multi_index<N(event), event> event_index;

    // @abi table runner i64
    struct runner {
        uint64_t runner_id;
        uint64_t event_id;
        std::string runner_name;

        uint64_t primary_key() const { return runner_id; }

        EOSLIB_SERIALIZE(runner, (runner_id)(event_id)(runner_name))
    };

    typedef eosio::multi_index<N(runner), runner> runner_index;

    // @abi table offer i64
    struct offer {
        uint64_t runner_id;
        double_t price;
        double_t max_amount;

        uint64_t primary_key() const { return runner_id; }

        EOSLIB_SERIALIZE(offer, (runner_id)(price)(max_amount))
    };

    typedef eosio::multi_index<N(offer), offer> offer_index;

    // @abi action
    void updevents(const std::vector<event> &events_to_update) {
        require_auth(_self);

        for (auto const &e: events_to_update) {

            auto itr = events.find(e.event_id);
            if (itr != events.end()) {
                events.modify(itr, 0, [&](auto &ev) {
                    ev.event_name = e.event_name;
                });
            } else {
                events.emplace(_self, [&](auto &ev) {
                    ev.event_id = e.event_id;
                    ev.event_name = e.event_name;
                });
            }
        }
    }

    // @abi action
    void updrunners(const std::vector<runner> &runners_to_update) {
        require_auth(_self);

        for (auto const &r: runners_to_update) {
            std::string rstr = r.runner_name + " -> " + std::to_string(r.runner_id) + "\n";
            eosio::print(rstr);

            auto itr = runners.find(r.runner_id);
            if (itr != runners.end()) {
                runners.modify(itr, 0, [&](auto &rn) {
                    rn.runner_name = r.runner_name;
                });
            } else {
                runners.emplace(_self, [&](auto &rn) {
                    rn.runner_id = r.runner_id;
                    rn.event_id = r.event_id;
                    rn.runner_name = r.runner_name;
                });
            }
        }
    }

    // @abi action
    void updoffers(const std::vector<offer>& offers_to_update) {
        require_auth(_self);

        for (auto const &o: offers_to_update) {

            auto itr = offers.find(o.runner_id);
            if (itr != offers.end()) {
                offers.modify(itr, 0, [&](auto &of) {
                    of.price = o.price;
                    of.max_amount = o.max_amount;
                });
            } else {
                offers.emplace(_self, [&](auto &of) {
                    of.runner_id = o.runner_id;
                    of.price = o.price;
                    of.max_amount = o.max_amount;
                });
            }
       }
    }

private:

    event_index events;
    runner_index runners;
    offer_index offers;

};

EOSIO_ABI(eossportbook, (updrunners)(updoffers)(updevents))
