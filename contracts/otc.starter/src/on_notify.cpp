void starter::otc::on_transfer( const name from, const name to, const asset quantity, const string memo )
{
    settings_singleton settings_table( get_self(), get_self().value );
    auto settings = settings_table.get();

    // ignore transfers
    const set<name> ignore = set<name>{
            // EOSIO system accounts
            "eosio.stake"_n,
            "eosio.names"_n,
            "eosio.ram"_n,
            "eosio.rex"_n,
            "eosio"_n
    };

    if ( from == get_self() || ignore.find( from ) != ignore.end() ) return;

    check( settings.enabled, "contract disabled" );

    require_auth( from );
    check(to == get_self(), "contract not involved in transfer");
    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "must transfer positive quantity" );

    // offer memo format example "offer,0.40000000 PBTC@btc.ptoken,0.00020000 PBTC"
    // just use offer id to buy
    auto memo_parts = split( memo, "," );
    check( (memo_parts.size() > 0), "memo not provided" );

    if (memo_parts.size() == 2) {
        // validate "buy"
        buy( from, quantity, memo_parts );

    } else {
        // validate "offer"
        check( (memo_parts.size() == 4), "invalid memo format" );
        // additional checks done when trying to add offer
        add_offer( from, quantity, memo_parts );
    }
}
