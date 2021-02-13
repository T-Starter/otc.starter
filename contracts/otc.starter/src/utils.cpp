vector<string> starter::otc::split(const string& str, const string& delim) {
    vector<string> parts;
    size_t prev = 0;
    size_t pos = 0;

    do {
        pos = str.find(delim, prev);
        if (pos == string::npos) pos = str.length();
        string part = str.substr(prev, pos-prev);
        parts.push_back(part);
        prev = pos + delim.length();
    }
    while (pos < str.length() && prev < str.length());
    return parts;
}

asset starter::otc::string_to_asset( const name& contract, const string& str ) {

    // "0.40000000 PBTC" -> "0.40000000", "PBTC"
    auto asset_parts = split( str, " " );

    auto asset_symbol_code = symbol_code(asset_parts[1].c_str());
    auto quantity = starter::otc::stod(asset_parts[0].c_str());

    // Validate asset against definition in stat table
    stats statstable( contract, asset_symbol_code.raw() );
    auto existing = statstable.find( asset_symbol_code.raw() );
    check( existing != statstable.end(), "symbol not defined for " + str );
    const auto& st = *existing;
    auto out_asset = double_to_asset(quantity, st.max_supply.symbol );

    return out_asset;
}

extended_asset starter::otc::string_to_extended_asset( const string& str ) {

    // example "0.5000000 PBTC@btc.ptokens" -> "0.5000000 PBTC", "btc.ptokens"
    auto str_parts = split( str, "@" );

    // btc.ptokens
    auto asset_contract_name = name(str_parts[1].c_str());
    // 0.40000000 PBTC
    auto asset_string = str_parts[0].c_str();
    auto out_asset = string_to_asset(asset_contract_name, asset_string );

    return { out_asset, asset_contract_name };
}

/*
 * Convert a SHA256 sum (given as a string) to checksum256 object
 */
bool starter::otc::hex2bin(const std::string& in, checksum256 &out)
{
    if (in.size() != 64)
        return false;

    std::string hex{"0123456789abcdef"};
    for (int i = 0; i < 32; i++) {
        auto d1 = hex.find(in[2*i]);
        auto d2 = hex.find(in[2*i+1]);
        if (d1 == std::string::npos || d2 == std::string::npos)
            return false;

        // checksum256 is composed of little endian int128_t
        reinterpret_cast<char *>(out.data())[i/16*16 + 15-(i%16)] = (d1 << 4)  + d2;
    }
    return true;
}


/*
 * calculate transaction hash for this transaction (trx_id)
 */
checksum256 starter::otc::get_trx_id()
{
    size_t size = transaction_size();
    char buf[size];
    size_t read = read_transaction( buf, size );
    check( size == read, "read_transaction failed");
    return sha256( buf, read );
}


