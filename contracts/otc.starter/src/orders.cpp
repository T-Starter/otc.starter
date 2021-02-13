void starter::otc::cancelorder( uint64_t id )
{
    require_auth( get_self() );
}

void starter::otc::add_offer( const name from, const asset quantity, vector<string> memo_parts ) {

    settings_singleton settings_table( get_self(), get_self().value );
    orders_table orders( get_self(), get_self().value );

    auto settings = settings_table.get();
    settings.last_order_id += 1;
    const auto order_id = settings.last_order_id;

    auto ask_contract = get_first_receiver();
    check (memo_parts[0] == std::string("offer"), "invalid memo format");
    checksum256 hash;
    check(hex2bin(memo_parts[3], hash), "invalid hash");

    auto ask = string_to_extended_asset( memo_parts[1] );
    auto min_ask = string_to_asset( ask.contract, memo_parts[2] );

    orders.emplace(get_self(), [&](auto &x) {
        x.id = order_id;
        x.maker = from;
        x.ask = ask;
        x.offer = { quantity, ask_contract };
        x.minimum = min_ask;
        x.remaining_ask = ask.quantity;
        x.remaining_offer = quantity;
        x.hash = hash;
    });

    settings_table.set(settings, get_self());
}

void starter::otc::buy( const name from, const asset quantity, vector<string> memo_parts ) {

    settings_singleton settings_table( get_self(), get_self().value );
    orders_table orders( get_self(), get_self().value );

    auto settings = settings_table.get();

    // order id in memo_parts[0]
    auto order_id = stoi(memo_parts[0]);
    check(order_id > 0, "order id must be positive");
    check(order_id <= settings.last_order_id, "invalid order id");

    auto orders_itr = orders.find( order_id );
    check( orders_itr != orders.end(), "order not found");
    auto order = *orders_itr;

    checksum256 hash = sha256((char *)memo_parts[1].c_str(), memo_parts[1].size());
    check(order.hash == hash, "incorrect key provided");

    check( order.remaining_offer.amount > 0, "sale finished");
    check( quantity >= order.minimum, "minimum purchase " + order.minimum.to_string() );

    loghistory( "purchase"_n, from, get_self(), { quantity, get_first_receiver() } );

    asset purchase;
    asset payment;

    if (quantity >= order.remaining_ask) {
        // final purchase
        if (quantity > order.remaining_ask) {
            // return surplus funds
            auto refund = quantity - order.remaining_ask;
            // change
            action( permission_level{ get_self(), "active"_n },
                    order.ask.contract, "transfer"_n,
                    std::make_tuple(get_self(), from, refund, string("refund surplus funds")) ).send();
            loghistory( "refund"_n, get_self(), from, { refund, order.ask.contract } );
        }

        purchase = order.remaining_offer;
        payment = order.remaining_ask;

    } else {
        // partial purchase
        auto payment_amount = asset_to_double( quantity );
        auto total_amount = asset_to_double( order.remaining_ask );
        auto f = payment_amount / total_amount;
        purchase = double_to_asset( f * asset_to_double( order.remaining_offer ), order.remaining_offer.symbol );
        payment = quantity;
    }

    // deliver purchase
    action(permission_level{get_self(), "active"_n},
           order.offer.contract, "transfer"_n,
           std::make_tuple(get_self(), from, purchase, string("tokens purchased"))).send();
    loghistory( "delivery"_n, get_self(), from, { purchase, order.offer.contract } );
    // pay seller
    action(permission_level{get_self(), "active"_n},
           order.ask.contract, "transfer"_n,
           std::make_tuple(get_self(), order.maker, payment, string("proceeds of sale"))).send();
    loghistory( "sale"_n, get_self(), order.maker, { payment, order.ask.contract } );

    orders.modify( orders_itr, same_payer, [&]( auto& x ) {
        x.remaining_ask -= payment;
        x.remaining_offer -= purchase;
    });
}
