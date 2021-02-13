void starter::otc::init( const name& admin, const bool& enabled ) {

    settings_singleton settings_table( get_self(), get_self().value );

    require_auth(get_self());

    check(!settings_table.exists(), "contract already initialised");

    settings_table.set(
            settings{
                    .admin = admin,
                    .enabled = enabled,
                    .last_order_id = 0
            },
            get_self());
}
