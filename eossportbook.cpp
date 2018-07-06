#include <eosiolib/eosio.hpp>
#include <eosiolib/serialize.hpp>

class eossportbook : public eosio::contract {
public:
    eossportbook(account_name self) :eosio::contract(self), events(_self, _self) {}

    /// @abi table event i64
    struct event{
        uint64_t event_id;
        std::string event_name;

        uint64_t primary_key()const { return event_id; }

        EOSLIB_SERIALIZE(event, (event_id)(event_name) )
    };

    typedef eosio::multi_index< N(event), event > event_index;

    struct runner {
        uint64_t runner_id;
        uint64_t event_id;
        std::string runner_name;
    };

    struct offer {
        uint64_t runner_id;
        double_t price;
        double_t amount;
    };

    /// @abi action
    void updevents(const std::vector<event>& events_to_update){
        require_auth(_self);
        eosio::print("-------> Events: ", events_to_update.size(), "\n");

        for(auto const& e: events_to_update){
            std::string rstr = e.event_name+" -> "+std::to_string(e.event_id)+"\n";
            eosio::print(rstr);

            auto itr = events.find(e.event_id);
            if(itr != events.end()){
                eosio::print("-----> Exist, update");
                events.modify(itr, 0, [&]( auto& ev){
                    ev.event_name = e.event_name;
                });
            }else{
                eosio::print("-----> Not exist, insert");
                events.emplace(_self, [&]( auto& ev){
                    ev.event_id = e.event_id;
                    ev.event_name = e.event_name;
                });
            }
        }
    }

    /// @abi action
    void updrunners(const std::vector<runner>& runners){
        eosio::print("-------> Runners: ", runners.size(), "\n");

        for(auto const& r: runners){
            std::string rstr = r.runner_name+" -> "+std::to_string(r.runner_id)+"\n";
            eosio::print(rstr);
        }
    }

    /// @abi action
    void updoffers(const std::vector<offer>& offers){
        for(auto const& o: offers){
            std::string ostr = std::to_string(o.runner_id)+" "+std::to_string(o.price)+" "+std::to_string(o.amount);
            eosio::print(ostr);
        }
    }

private:

    event_index events;

};

EOSIO_ABI( eossportbook, (updrunners)(updoffers)(updevents) )
