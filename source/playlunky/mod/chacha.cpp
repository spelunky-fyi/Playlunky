#include "chacha.h"

#include <array>
#include <cassert>

namespace ChaCha {
    // This implementation is a direct translation of Modlunky's chacha source
    // For reference see: https://github.com/spelunky-fyi/modlunky2/blob/1dd9acf7ae779d7ead82e7d74e2319dd46fef909/src/modlunky2/assets/chacha.py

    using words_t = std::vector<std::uint32_t>;
    using qwords_t = std::vector<std::uint64_t>;

    using s_bytes_t = std::array<uint8_t, 64>;

    using s_t = std::span<std::uint8_t, 64>;
    using w_t = std::span<std::uint32_t, 16>;
    using q_t = std::span<std::uint64_t, 8>;

    w_t s_to_w(s_t s) {
        return w_t{ reinterpret_cast<std::uint32_t*>(s.data()), s.size() / 4 };
    }

    s_t w_to_s(w_t w) {
        return s_t{ reinterpret_cast<std::uint8_t*>(w.data()), w.size() * 4 };
    }

    q_t s_to_q(s_t s) {
        return q_t{ reinterpret_cast<std::uint64_t*>(s.data()), s.size() / 8 };
    }

    s_t q_to_s(q_t q) {
        return s_t{ reinterpret_cast<std::uint8_t*>(q.data()), q.size() * 8 };
    }

    template<class T>
    T rotate_left(T a, int b) {
        // Equivalent to:
        // return std::rotl(a, b);
        static constexpr int bits = static_cast<int>(sizeof(T) * 8);
        return (a << b) | (a >> (bits - b));
    }

    void quarter_round(w_t w, std::size_t a, std::size_t b, std::size_t c, std::size_t d) {
        w[a] += w[b];
        w[d] ^= w[a];
        w[d] = rotate_left(w[d], 16);
        w[c] += w[d];
        w[b] ^= w[c];
        w[b] = rotate_left(w[b], 12);
        w[a] += w[b];
        w[d] ^= w[a];
        w[d] = rotate_left(w[d], 8);
        w[c] += w[d];
        w[b] ^= w[c];
        w[b] = rotate_left(w[b], 7);
    }

    void round_pair(w_t w) {
        quarter_round(w, 0, 4, 8, 12);
        quarter_round(w, 1, 5, 9, 13);
        quarter_round(w, 2, 6, 10, 14);
        quarter_round(w, 3, 7, 11, 15);
        quarter_round(w, 0, 5, 10, 15);
        quarter_round(w, 1, 6, 11, 12);
        quarter_round(w, 2, 7, 8, 13);
        quarter_round(w, 3, 4, 9, 14);
    }
    
    s_t two_rounds(s_t s) {
        w_t w = s_to_w(s);
        round_pair(w);
        round_pair(w);
        return w_to_s(w);
    }

    s_t quad_rounds(s_t s) {
        w_t w = s_to_w(s);
        round_pair(w);
        round_pair(w);
        round_pair(w);
        round_pair(w);
        return w_to_s(w);
    }

    bytes_t sxor(std::span<const std::uint8_t> x, std::span<const std::uint8_t> y) {
        assert(x.size() == y.size());

        bytes_t bytes(x.size());
        for (size_t i = 0; i < bytes.size(); i++) {
            bytes[i] = x[i] ^ y[y.size() - 1 - i];
        }
        return bytes;
    }

    s_bytes_t add_qwords(s_t s0, s_t s1) {
        q_t q0 = s_to_q(s0);
        q_t q1 = s_to_q(s1);
        qwords_t qwords(q0.size());
        for (size_t i = 0; i < qwords.size(); i++) {
            qwords[i] = q0[i] + q1[i];
        }
        s_bytes_t bytes;
        memcpy(bytes.data(), qwords.data(), qwords.size() * 8);
        return bytes;
    }
    
    s_t mix_in(s_t h, std::string_view s) {
        auto mix_partial = [](s_t h, std::string_view partial) -> s_t {
            assert(partial.size() <= h.size());
            for (size_t i = 0; i < partial.size(); i++) {
                const auto inverse_i = partial.size() - 1 - i;
                h[i] ^= static_cast<int>(partial[inverse_i]);
            }
            return quad_rounds(h);
        };

        while(!s.empty()) {
            h = mix_partial(h, s.substr(0, 0x40));
            s = s.substr(std::min(s.size(), (std::size_t)0x40));
        }

        return h;
    }

    bytes_t keyed_hashing(std::string_view filepath, s_t key) {
        // Do keyed hashing
        // NOTE: This appears to be an implementation mistake on the Spelunky 2 dev's part
        // They generate a quad_round advanced version of (nonce'd key), but then they
        // xor with the untweaked key instead of the tweaked key...

        bytes_t h;
        for (std::size_t i = 0; i < filepath.size(); i += 0x40) {
            std::string_view partial = filepath.substr(i, i + 0x40);
            std::string partial_mutable{ partial.begin(), partial.end() };
            std::span<std::uint8_t> s_partial{ reinterpret_cast<std::uint8_t*>(partial_mutable.data()), reinterpret_cast<std::uint8_t*>(partial_mutable.data()) + partial_mutable.size() };
            bytes_t bytes = sxor(s_partial, key.subspan(0, partial.size()));
            h.insert(h.end(), bytes.begin(), bytes.end());
        }
        return h;
    }

    bytes_t hash_filepath_v1(std::string_view filepath) {
        // Generate initial hash from the string
        bytes_t bytes(16, 0);
        const s_t h0{ bytes };
        const s_t h1 = mix_in(h0, filepath);

        // Advance h1 by four round pairs to get h2
        const s_t h2 = quad_rounds(h1);
        // Add the two together, and advance by four round pairs.
        s_bytes_t added_bytes = add_qwords(h1, h2);
        const s_t h3{ added_bytes };
        s_t key = quad_rounds(h3);

        return keyed_hashing(filepath, key);
    }

    bytes_t hash_filepath_v2(std::string_view filepath, std::uint64_t key) {
        // Generate initial hash from the string
        qwords_t qwords{ key, filepath.size(), 0, 0, 0, 0, 0, 0 };
        assert(qwords.size() == 8);
        s_t h = two_rounds(q_to_s(q_t{ qwords }));
        h = mix_in(h, filepath);

        // Add the two together, and advance by four round pairs.
        s_bytes_t h_copy;
        memcpy(h_copy.data(), h.data(), h.size());
        s_bytes_t tmp_s = add_qwords(h, quad_rounds(s_t{ h_copy }));
        q_t tmp_q = s_to_q(s_t{ tmp_s });
        tmp_q[0] ^= filepath.size();
        s_t key_s = quad_rounds(q_to_s(tmp_q));

        return keyed_hashing(filepath, key_s);
    }

    bytes_t hash_filepath(std::string_view filepath, std::uint64_t key, Version version) {
        if (version == Version::V1) {
            return hash_filepath_v1(filepath);
        }
        else {
            return hash_filepath_v2(filepath, key);
        }
    }

    s_t mix_in_filepath(std::string_view filepath, s_t h) {
        // Mix the filename in to tweak the key
        for (std::size_t i = 0; i < filepath.size(); i += 0x40) {
            std::string_view partial = filepath.substr(i, i + 0x40);
            std::string partial_mutable{ partial.begin(), partial.end() };
            std::span<std::uint8_t> s_partial{ reinterpret_cast<std::uint8_t*>(partial_mutable.data()), reinterpret_cast<std::uint8_t*>(partial_mutable.data()) + partial_mutable.size() };
            bytes_t bytes = sxor(h.subspan(0, partial.size()), s_partial);
            for (size_t j = 0; j < bytes.size(); j++) {
                h[j] = bytes[j];
            }
            h = quad_rounds(h);
        }
        return h;
    }
    
    bytes_t chacha_rest(std::span<const std::uint8_t> data, s_t key) {
        // NOTE: This appears to be an implementation mistake on the Spelunky 2 dev's part
        // They generate a quad_round advanced version of (nonce'd key), but then they
        // xor with the untweaked key instead of the tweaked key...
        bytes_t out;
        for (std::size_t i = 0; i < data.size(); i += 0x40) {
            std::span<const std::uint8_t> partial = data.subspan(i, std::min((std::size_t)0x40, data.size() - i));
            bytes_t bytes = sxor(partial, key.subspan(0, partial.size()));
            out.insert(out.end(), bytes.begin(), bytes.end());
        }
        return out;
    }

    bytes_t chacha_v1(std::string_view filepath, std::span<const std::uint8_t> data) {
        // Untweaked key begins as half - advanced "0xBABE"
        qwords_t qwords{ 0xBABE, 0, 0, 0, 0, 0, 0, 0 };
        assert(qwords.size() == 8);
        s_t h = two_rounds(q_to_s(q_t{ qwords }));

        h = mix_in_filepath(filepath, h);

        // Add the tweaked key and its advancement, then advance by four round pairs.
        s_bytes_t h_copy;
        memcpy(h_copy.data(), h.data(), h.size());
        s_bytes_t bytes = add_qwords(h, quad_rounds(s_t{ h_copy }));
        s_t key = quad_rounds(s_t{ bytes });

        return chacha_rest(data, key);
    }

    bytes_t chacha_v2(std::string_view filepath, std::span<const std::uint8_t> data, std::uint64_t key) {
        // Untweaked key begins as half - advanced `key`
        qwords_t qwords{ key, filepath.size(), 0, 0, 0, 0, 0, 0 };
        assert(qwords.size() == 8);
        s_t h = two_rounds(q_to_s(q_t{ qwords }));

        h = mix_in_filepath(filepath, h);

        // Add the tweaked key and its advancement, then advance by four round pairs.
        s_bytes_t h_copy;
        memcpy(h_copy.data(), h.data(), h.size());
        s_bytes_t tmp = add_qwords(h, quad_rounds(s_t{ h_copy }));
        q_t q_tmp = s_to_q(s_t{ tmp });
        q_tmp[0] = q_tmp[0] ^ key + data.size();
        
        s_t key_s = quad_rounds(q_to_s(q_tmp));

        return chacha_rest(data, key_s);
    }

    bytes_t chacha(std::string_view filepath, std::span<const std::uint8_t> data, std::uint64_t key, Version version) {
        if (version == Version::V1) {
            return chacha_v1(filepath, data);
        }
        else {
            return chacha_v2(filepath, data, key);
        }
    }

    void Key::update(std::uint64_t asset_len) {
        const auto v3 = (
            0x9E6C63D0676A9A99 * (
                Current
                ^ asset_len
                ^ rotate_left(Current ^ asset_len, 17)
                ^ rotate_left(Current ^ asset_len, 64 - 25)
            )
        );
        const auto v4 = 0x9E6D62D06F6A9A9B * (v3 ^ ((v3 ^ (v3 >> 28)) >> 23));
        Current ^= v4 ^ ((v4 ^ (v4 >> 28)) >> 23);
    }
}