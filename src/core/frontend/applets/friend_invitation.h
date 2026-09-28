// SPDX-FileCopyrightText: Copyright 2026 citron Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <functional>
#include <optional>
#include <span>
#include <vector>
#include "common/common_types.h"
#include "core/frontend/applets/applet.h"

namespace Core::Frontend {

struct FriendInvitationRequest {
    u32 mode{};
    std::array<u8, 16> user_id{};
    u32 recipient_limit{};
    std::vector<u64> account_ids;
    std::vector<u8> user_data;
    std::array<u8, 0xC00> description{};
    u64 invitation_id{};
    u64 group_id{};
};

// MyPage's 9.0.0+ input ABI. Decode explicitly instead of casting an untrusted
// storage buffer to a struct (alignment and truncated input must both be safe).
inline std::optional<FriendInvitationRequest> DecodeFriendInvitation(
    std::span<const u8> input) {
    if (input.size() < 0x18) {
        return std::nullopt;
    }
    const auto read = [&](size_t offset, size_t bytes) {
        u64 value{};
        for (size_t i = 0; i < bytes; ++i) {
            value |= static_cast<u64>(input[offset + i]) << (8 * i);
        }
        return value;
    };
    FriendInvitationRequest result;
    result.mode = static_cast<u32>(read(0, 4));
    for (size_t i = 0; i < result.user_id.size(); ++i) {
        result.user_id[i] = input[8 + i];
    }
    if (result.mode == 10) {
        if (input.size() < 0x28) {
            return std::nullopt;
        }
        result.invitation_id = read(0x18, 8);
        result.group_id = read(0x20, 8);
        return result;
    }
    if (result.mode != 8 && result.mode != 9) {
        return std::nullopt;
    }
    const size_t size_offset = result.mode == 8 ? 0x20 : 0xA0;
    const size_t data_offset = size_offset + 8;
    const size_t description_offset = data_offset + 0x400;
    if (input.size() < description_offset + result.description.size()) {
        return std::nullopt;
    }
    result.recipient_limit = static_cast<u32>(read(0x18, 4));
    if (result.recipient_limit == 0 || result.recipient_limit > 16) {
        return std::nullopt;
    }
    const auto data_size = read(size_offset, 8);
    if (data_size > 0x400) {
        return std::nullopt;
    }
    if (result.mode == 9) {
        for (u32 i = 0; i < result.recipient_limit; ++i) {
            result.account_ids.push_back(read(0x20 + i * 8, 8));
        }
    }
    result.user_data.assign(input.begin() + data_offset,
                            input.begin() + data_offset + data_size);
    for (size_t i = 0; i < result.description.size(); ++i) {
        result.description[i] = input[description_offset + i];
    }
    return result;
}

class FriendInvitationApplet : public Applet {
public:
    // Result is the applet's wire result, not an assertion of delivery.
    using Completion = std::function<void(u32)>;
    virtual void ShowInvitation(const FriendInvitationRequest& request, u64 title_id,
                                Completion completion) const = 0;
};

} // namespace Core::Frontend
