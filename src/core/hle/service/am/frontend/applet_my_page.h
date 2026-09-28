// SPDX-FileCopyrightText: Copyright 2026 citron Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <atomic>
#include "core/frontend/applets/friend_invitation.h"
#include "core/hle/result.h"
#include "core/hle/service/am/frontend/applets.h"

namespace Service::AM::Frontend {
class MyPage final : public FrontendApplet, public std::enable_shared_from_this<MyPage> {
public:
    MyPage(Core::System& system, std::shared_ptr<Applet> applet, LibraryAppletMode mode,
           const Core::Frontend::FriendInvitationApplet* frontend);
    void Initialize() override;
    Result GetStatus() const override { return ResultSuccess; }
    void ExecuteInteractive() override {}
    void Execute() override;
    Result RequestExit() override;
private:
    void Complete(u32 result);
    const Core::Frontend::FriendInvitationApplet* frontend;
    std::optional<Core::Frontend::FriendInvitationRequest> request;
    bool started{};
    std::atomic<bool> complete{};
};
}
