// SPDX-FileCopyrightText: Copyright 2026 citron Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
#include "core/core.h"
#include "core/hle/service/am/frontend/applet_my_page.h"
#include "core/hle/service/am/service/storage.h"

namespace Service::AM::Frontend {
MyPage::MyPage(Core::System& system_, std::shared_ptr<Applet> applet_, LibraryAppletMode mode,
               const Core::Frontend::FriendInvitationApplet* frontend_)
    : FrontendApplet{system_, applet_, mode}, frontend{frontend_} {}

void MyPage::Initialize() {
    FrontendApplet::Initialize();
    if (const auto input = PopInData()) {
        request = Core::Frontend::DecodeFriendInvitation(input->GetData());
    }
}

void MyPage::Execute() {
    if (started || complete) return;
    started = true;
    if (!request || !frontend) { Complete(ResultUnknown.raw); return; }
    frontend->ShowInvitation(*request, system.GetApplicationProcessProgramID(),
        [weak = weak_from_this()](u32 result) {
            if (auto self = weak.lock()) self->Complete(result);
        });
}

void MyPage::Complete(u32 result) {
    if (complete.exchange(true) || applet.expired()) return;
    if (!request || request->mode == 8 || request->mode == 9) {
        std::vector<u8> output(4);
        for (size_t i = 0; i < 4; ++i) output[i] = static_cast<u8>(result >> (8 * i));
        PushOutData(std::make_shared<IStorage>(system, std::move(output)));
    }
    Exit();
}

Result MyPage::RequestExit() {
    if (frontend) frontend->Close();
    Complete(ResultUnknown.raw);
    R_SUCCEED();
}
}
