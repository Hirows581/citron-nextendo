// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDialog>

#include "core/frontend/applets/software_keyboard.h"

class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;

namespace Core {
class System;
}

class GMainWindow;

// A plain pop-up text box standing in for the Switch software keyboard.
class QtSoftwareKeyboardDialog final : public QDialog {
    Q_OBJECT

public:
    QtSoftwareKeyboardDialog(QWidget* parent, Core::System& system_, bool is_inline_,
                             Core::Frontend::KeyboardInitializeParameters initialize_parameters_);
    ~QtSoftwareKeyboardDialog() override;

    void ShowNormalKeyboard(QPoint pos, QSize size);

    void ShowTextCheckDialog(Service::AM::Frontend::SwkbdTextCheckResult text_check_result,
                             std::u16string text_check_message);

    void ShowInlineKeyboard(Core::Frontend::InlineAppearParameters appear_parameters, QPoint pos,
                            QSize size);

    void HideInlineKeyboard();

    void InlineTextChanged(Core::Frontend::InlineTextParameters text_parameters);

    void ExitKeyboard();

signals:
    void SubmitNormalText(Service::AM::Frontend::SwkbdResult result, std::u16string submitted_text,
                          bool confirmed = false) const;

    void SubmitInlineText(Service::AM::Frontend::SwkbdReplyType reply_type,
                          std::u16string submitted_text, s32 cursor_position) const;

public slots:
    void accept() override;
    void reject() override;

private:
    void ShowCentered(QPoint pos, QSize size);
    void ApplyParameters();
    QString Text() const;
    void SetText(const QString& text);
    s32 CursorPosition() const;
    void OnTextEdited();

    /// Returns an empty string if the text is acceptable, else why it is not.
    QString ValidationError(const QString& text) const;

    bool is_inline;
    Core::Frontend::KeyboardInitializeParameters initialize_parameters;

    QLabel* header_label;
    QLabel* sub_label;
    QLineEdit* line_edit;
    QPlainTextEdit* text_edit;
    QLabel* status_label;
    QDialogButtonBox* buttons;
};

class QtSoftwareKeyboard final : public QObject, public Core::Frontend::SoftwareKeyboardApplet {
    Q_OBJECT

public:
    explicit QtSoftwareKeyboard(GMainWindow& parent);
    ~QtSoftwareKeyboard() override;

    void Close() const override {
        ExitKeyboard();
    }

    void InitializeKeyboard(bool is_inline,
                            Core::Frontend::KeyboardInitializeParameters initialize_parameters,
                            SubmitNormalCallback submit_normal_callback_,
                            SubmitInlineCallback submit_inline_callback_) override;

    void ShowNormalKeyboard() const override;

    void ShowTextCheckDialog(Service::AM::Frontend::SwkbdTextCheckResult text_check_result,
                             std::u16string text_check_message) const override;

    void ShowInlineKeyboard(Core::Frontend::InlineAppearParameters appear_parameters) override;

    void HideInlineKeyboard() const override;

    void InlineTextChanged(Core::Frontend::InlineTextParameters text_parameters) override;

    void ExitKeyboard() const override;

signals:
    void MainWindowInitializeKeyboard(
        bool is_inline, Core::Frontend::KeyboardInitializeParameters initialize_parameters) const;

    void MainWindowShowNormalKeyboard() const;

    void MainWindowShowTextCheckDialog(
        Service::AM::Frontend::SwkbdTextCheckResult text_check_result,
        std::u16string text_check_message) const;

    void MainWindowShowInlineKeyboard(
        Core::Frontend::InlineAppearParameters appear_parameters) const;

    void MainWindowHideInlineKeyboard() const;

    void MainWindowInlineTextChanged(Core::Frontend::InlineTextParameters text_parameters) const;

    void MainWindowExitKeyboard() const;

private:
    void SubmitNormalText(Service::AM::Frontend::SwkbdResult result, std::u16string submitted_text,
                          bool confirmed) const;

    void SubmitInlineText(Service::AM::Frontend::SwkbdReplyType reply_type,
                          std::u16string submitted_text, s32 cursor_position) const;

    mutable SubmitNormalCallback submit_normal_callback;
    mutable SubmitInlineCallback submit_inline_callback;
};
