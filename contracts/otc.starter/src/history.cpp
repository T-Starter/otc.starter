
void starter::otc::clearhistory( )
{
    require_auth( get_self() );
}

void starter::otc::loghistory( const name event_type, const name from, const name to, const extended_asset quantity )
{
    settings_singleton settings_table( get_self(), get_self().value );
    history_table history_log( get_self(), get_self().value );

    auto settings = settings_table.get();
    settings.last_log_id += 1;
    const auto log_id = settings.last_log_id;

    history_log.emplace(get_self(), [&](auto &x) {
        x.id = log_id;
        x.timestamp = eosio::time_point_sec(current_time_point());
        x.event_type = event_type;
        x.from = from;
        x.to = to;
        x.contract = quantity.contract;
        x.quantity = quantity.quantity;
        x.trxid = get_trx_id();
    });

    settings_table.set(settings, get_self());
}
