// SPDX-FileCopyrightText: Copyright 2026 citron Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QObject>
#include <QPointer>
#include "core/frontend/applets/friend_invitation.h"
class QWidget;
class QDialog;

class QtFriendInvitation final : public QObject, public Core::Frontend::FriendInvitationApplet {
public:
    explicit QtFriendInvitation(QWidget* parent);
    ~QtFriendInvitation() override;
    void Close() const override;
    void ShowInvitation(const Core::Frontend::FriendInvitationRequest& request, u64 title_id,
                        Completion completion) const override;
private:
    QPointer<QWidget> parent;
    mutable QPointer<QDialog> dialog;
};
