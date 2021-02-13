#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/symbol.hpp>
#include <eosio/singleton.hpp>
#include <eosio/crypto.hpp>
#include <eosio/transaction.hpp>

#include <math.h>

using namespace eosio;
using namespace std;

namespace starter {

    class [[eosio::contract("otc.starter")]] otc : public contract {

    public:
        using contract::contract;

        struct [[eosio::table("settings")]] settings {
            name                            admin;                // only admin will be able to create orders
            bool                            enabled;              // pause / enable operation
            uint64_t                        last_order_id;        // autoincrement order id
            uint64_t                        last_log_id;         // autoincrement log id
        };
        typedef eosio::singleton<"settings"_n, settings> settings_singleton;

        struct [[eosio::table("orders")]] orders_row {
            uint64_t                        id;
            name                            maker;
            extended_asset                  ask;
            extended_asset                  offer;
            asset                           minimum;
            asset                           remaining_ask;
            asset                           remaining_offer;
            checksum256                     hash;

            uint64_t primary_key() const { return id; }
        };
        typedef eosio::multi_index< "orders"_n, orders_row > orders_table;

        struct [[eosio::table]] history_row {
            uint64_t id;
            time_point_sec timestamp;
            name event_type;
            name from;
            name to;
            name contract;
            asset quantity;
            checksum256 trxid;

            uint64_t primary_key() const { return id; }
        };
        typedef eosio::multi_index< "history"_n, history_row > history_table;

        [[eosio::action]]
        void init( const name&   admin,
                   const bool&   enabled );

        [[eosio::action]]
        void cancelorder( uint64_t id );

        [[eosio::action]]
        void clearhistory( );

        [[eosio::action]]
        void loghistory( const name event_type, const name from, const name to, const extended_asset quantity );

        [[eosio::on_notify("*::transfer")]]
        void on_transfer( const name from, const name to, const asset quantity, const string memo );

        // action wrappers
        using init_action = eosio::action_wrapper<"init"_n, &starter::otc::init>;
        using cancelorder_action = eosio::action_wrapper<"cancelorder"_n, &starter::otc::cancelorder>;
        using clearhistory_action = eosio::action_wrapper<"clearhistory"_n, &starter::otc::clearhistory>;

    private:

        // Required to validate extended assets
        struct [[eosio::table]] currency_stats {
            asset    supply;
            asset    max_supply;
            name     issuer;

            uint64_t primary_key()const { return supply.symbol.code().raw(); }
        };
        typedef eosio::multi_index< "stat"_n, currency_stats > stats;

        // Simplify maths on asset conversions
        static double asset_to_double( const asset quantity ) {
            if ( quantity.amount == 0 ) return 0.0;
            return quantity.amount / pow(10, quantity.symbol.precision());
        }

        static asset double_to_asset( const double amount, const symbol sym ) {
            return asset{ static_cast<int64_t>(amount * pow(10, sym.precision())), sym };
        }

        double stod(const char* s) {
            double rez = 0, fact = 1;

            if (*s == '-') {
                s++;
                fact = -1;
            }
            for (int point_seen = 0; *s; s++) {
                if (*s == '.') {
                    if (point_seen) return 0;
                    point_seen = 1;
                    continue;
                }
                int d = *s - '0';
                if (d >= 0 && d <= 9) {
                    if (point_seen) fact /= 10.0f;
                    rez = rez * 10.0f + (double)d;
                } else return 0;
            }
            return rez * fact;
        }

        // utils
        asset string_to_asset( const name& contract, const string& str );
        extended_asset string_to_extended_asset( const string& str );
        vector<string> split(const string& str, const string& delim);
        bool hex2bin(const std::string& in, checksum256 &out);
        checksum256 get_trx_id();

        // orders
        void add_offer( const name from, const asset quantity, vector<string> memo_parts );
        void buy( const name from, const asset quantity, vector<string> memo_parts );
    };
}