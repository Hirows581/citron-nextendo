// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-FileCopyrightText: Copyright 2026 citron-neo Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>

#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTextCursor>
#include <QVBoxLayout>

#include "common/logging.h"
#include "common/string_util.h"
#include "core/core.h"
#include "citron/applets/qt_software_keyboard.h"
#include "citron/main.h"

namespace {

using namespace Service::AM::Frontend;

} // Anonymous namespace

QtSoftwareKeyboardDialog::QtSoftwareKeyboardDialog(
    QWidget* parent, [[maybe_unused]] Core::System& system_, bool is_inline_,
    Core::Frontend::KeyboardInitializeParameters initialize_parameters_)
    : QDialog(parent), is_inline{is_inline_},
      initialize_parameters{std::move(initialize_parameters_)} {
    setWindowTitle(tr("Software Keyboard"));
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint |
                   Qt::WindowStaysOnTopHint);
    setWindowModality(Qt::WindowModal);
    setAttribute(Qt::WA_DeleteOnClose);
    setMinimumWidth(460);

    header_label = new QLabel(this);
    QFont header_font = header_label->font();
    header_font.setPointSizeF(header_font.pointSizeF() * 1.3);
    header_font.setBold(true);
    header_label->setFont(header_font);
    header_label->setWordWrap(true);

    sub_label = new QLabel(this);
    sub_label->setWordWrap(true);

    line_edit = new QLineEdit(this);
    text_edit = new QPlainTextEdit(this);
    text_edit->setMinimumHeight(120);

    status_label = new QLabel(this);
    status_label->setWordWrap(true);

    buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(header_label);
    layout->addWidget(sub_label);
    layout->addWidget(line_edit);
    layout->addWidget(text_edit);
    layout->addWidget(status_label);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QtSoftwareKeyboardDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QtSoftwareKeyboardDialog::reject);
    connect(line_edit, &QLineEdit::textEdited, this, &QtSoftwareKeyboardDialog::OnTextEdited);
    connect(text_edit, &QPlainTextEdit::textChanged, this, &QtSoftwareKeyboardDialog::OnTextEdited);

    ApplyParameters();
    SetText(QString::fromStdU16String(initialize_parameters.initial_text));
}

QtSoftwareKeyboardDialog::~QtSoftwareKeyboardDialog() = default;

void QtSoftwareKeyboardDialog::ShowNormalKeyboard(QPoint pos, QSize size) {
    if (isVisible()) {
        return;
    }
    ShowCentered(pos, size);
}

void QtSoftwareKeyboardDialog::ShowTextCheckDialog(SwkbdTextCheckResult text_check_result,
                                                   std::u16string text_check_message) {
    const QString message = QString::fromStdU16String(text_check_message);
    switch (text_check_result) {
    case SwkbdTextCheckResult::Failure:
        QMessageBox::warning(this, windowTitle(), message);
        break;
    case SwkbdTextCheckResult::Confirm:
        if (QMessageBox::question(this, windowTitle(), message,
                                  QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Ok) {
            const QString text = Text();
            emit SubmitNormalText(SwkbdResult::Ok,
                                  Common::U16StringFromBuffer(text.utf16(), text.size()), true);
        }
        break;
    case SwkbdTextCheckResult::Success:
    case SwkbdTextCheckResult::Silent:
    default:
        break;
    }
}

void QtSoftwareKeyboardDialog::ShowInlineKeyboard(
    Core::Frontend::InlineAppearParameters appear_parameters, QPoint pos, QSize size) {
    initialize_parameters.max_text_length = appear_parameters.max_text_length;
    initialize_parameters.min_text_length = appear_parameters.min_text_length;
    initialize_parameters.type = appear_parameters.type;
    initialize_parameters.key_disable_flags = appear_parameters.key_disable_flags;
    initialize_parameters.enable_backspace_button = appear_parameters.enable_backspace_button;
    initialize_parameters.enable_return_button = appear_parameters.enable_return_button;
    initialize_parameters.disable_cancel_button = appear_parameters.disable_cancel_button;

    ApplyParameters();
    ShowCentered(pos, size);
}

void QtSoftwareKeyboardDialog::HideInlineKeyboard() {
    QDialog::hide();
}

void QtSoftwareKeyboardDialog::InlineTextChanged(
    Core::Frontend::InlineTextParameters text_parameters) {
    const QString text = QString::fromStdU16String(text_parameters.input_text);
    if (text != Text()) {
        SetText(text);
    }
}

void QtSoftwareKeyboardDialog::ExitKeyboard() {
    QDialog::done(QDialog::Accepted);
}

void QtSoftwareKeyboardDialog::accept() {
    const QString text = Text();
    if (!ValidationError(text).isEmpty()) {
        return;
    }

    auto text_str = Common::U16StringFromBuffer(text.utf16(), text.size());
    if (is_inline) {
        emit SubmitInlineText(SwkbdReplyType::ChangedString, text_str, CursorPosition());
        emit SubmitInlineText(SwkbdReplyType::DecidedEnter, std::move(text_str), CursorPosition());
    } else {
        emit SubmitNormalText(SwkbdResult::Ok, std::move(text_str));
    }
}

void QtSoftwareKeyboardDialog::reject() {
    // Escape and the close button follow the game's rule on whether cancelling is allowed.
    if (initialize_parameters.disable_cancel_button) {
        return;
    }

    const QString text = Text();
    auto text_str = Common::U16StringFromBuffer(text.utf16(), text.size());
    if (is_inline) {
        emit SubmitInlineText(SwkbdReplyType::DecidedCancel, std::move(text_str), CursorPosition());
    } else {
        emit SubmitNormalText(SwkbdResult::Cancel, std::move(text_str));
    }
}

void QtSoftwareKeyboardDialog::ShowCentered(QPoint pos, QSize size) {
    adjustSize();
    move(pos.x() + (size.width() - width()) / 2, pos.y() + (size.height() - height()) / 2);

    show();
    raise();
    activateWindow();
    if (text_edit->isVisible()) {
        text_edit->setFocus(Qt::OtherFocusReason);
    } else {
        line_edit->setFocus(Qt::OtherFocusReason);
    }
}

void QtSoftwareKeyboardDialog::ApplyParameters() {
    const auto& params = initialize_parameters;

    QString header = QString::fromStdU16String(params.header_text);
    const QString sub = QString::fromStdU16String(params.sub_text);
    if (header.isEmpty() && sub.isEmpty()) {
        header = tr("Enter text");
    }
    header_label->setText(header);
    header_label->setVisible(!header.isEmpty());
    sub_label->setText(sub);
    sub_label->setVisible(!sub.isEmpty());

    const bool multiline = !is_inline && params.text_draw_type == SwkbdTextDrawType::Box;
    line_edit->setVisible(!multiline);
    text_edit->setVisible(multiline);

    const QString guide = QString::fromStdU16String(params.guide_text);
    line_edit->setPlaceholderText(guide);
    text_edit->setPlaceholderText(guide);

    line_edit->setEchoMode(params.password_mode == SwkbdPasswordMode::Enabled
                               ? QLineEdit::Password
                               : QLineEdit::Normal);
    if (params.max_text_length > 0) {
        line_edit->setMaxLength(static_cast<int>(params.max_text_length));
    }

    const QString ok_text = QString::fromStdU16String(params.ok_text);
    buttons->button(QDialogButtonBox::Ok)->setText(ok_text.isEmpty() ? tr("OK") : ok_text);
    buttons->button(QDialogButtonBox::Cancel)->setVisible(!params.disable_cancel_button);

    OnTextEdited();
}

QString QtSoftwareKeyboardDialog::Text() const {
    return text_edit->isVisibleTo(this) ? text_edit->toPlainText() : line_edit->text();
}

void QtSoftwareKeyboardDialog::SetText(const QString& text) {
    const QSignalBlocker line_blocker(line_edit);
    const QSignalBlocker text_blocker(text_edit);
    line_edit->setText(text);
    text_edit->setPlainText(text);

    const bool at_start = initialize_parameters.initial_cursor_position == 0;
    line_edit->setCursorPosition(at_start ? 0 : static_cast<int>(text.size()));
    auto cursor = text_edit->textCursor();
    cursor.movePosition(at_start ? QTextCursor::Start : QTextCursor::End);
    text_edit->setTextCursor(cursor);

    const QString error = ValidationError(Text());
    status_label->setText(error);
    buttons->button(QDialogButtonBox::Ok)->setEnabled(error.isEmpty());
}

s32 QtSoftwareKeyboardDialog::CursorPosition() const {
    return text_edit->isVisibleTo(this) ? text_edit->textCursor().position()
                                        : line_edit->cursorPosition();
}

void QtSoftwareKeyboardDialog::OnTextEdited() {
    QString text = Text();
    const auto max_length = static_cast<qsizetype>(initialize_parameters.max_text_length);
    if (text_edit->isVisibleTo(this) && max_length > 0 && text.size() > max_length) {
        const QSignalBlocker blocker(text_edit);
        text.truncate(max_length);
        text_edit->setPlainText(text);
        text_edit->moveCursor(QTextCursor::End);
    }

    const QString error = ValidationError(text);
    status_label->setText(error);
    buttons->button(QDialogButtonBox::Ok)->setEnabled(error.isEmpty());

    if (is_inline && isVisible()) {
        emit SubmitInlineText(SwkbdReplyType::ChangedString,
                              Common::U16StringFromBuffer(text.utf16(), text.size()),
                              CursorPosition());
    }
}

QString QtSoftwareKeyboardDialog::ValidationError(const QString& text) const {
    const auto& params = initialize_parameters;
    const auto& flags = params.key_disable_flags;
    const auto length = static_cast<u32>(text.size());

    if (length < params.min_text_length) {
        return tr("Enter at least %n character(s).", "", static_cast<int>(params.min_text_length));
    }
    if (params.max_text_length > 0 && length > params.max_text_length) {
        return tr("Enter at most %n character(s).", "", static_cast<int>(params.max_text_length));
    }
    if (flags.space && text.contains(QLatin1Char{' '})) {
        return tr("Spaces are not allowed.");
    }
    if ((flags.at || flags.username) && text.contains(QLatin1Char{'@'})) {
        return tr("\"@\" is not allowed.");
    }
    if ((flags.percent || flags.username) && text.contains(QLatin1Char{'%'})) {
        return tr("\"%\" is not allowed.");
    }
    if (flags.slash && text.contains(QLatin1Char{'/'})) {
        return tr("\"/\" is not allowed.");
    }
    if ((flags.backslash || flags.username) && text.contains(QLatin1Char{'\\'})) {
        return tr("\"\\\" is not allowed.");
    }
    if (flags.numbers && std::any_of(text.begin(), text.end(), [](QChar c) { return c.isDigit(); })) {
        return tr("Numbers are not allowed.");
    }
    if (params.type == SwkbdType::NumberPad &&
        std::any_of(text.begin(), text.end(), [&params](QChar c) {
            return !c.isDigit() && c != QChar{params.left_optional_symbol_key} &&
                   c != QChar{params.right_optional_symbol_key};
        })) {
        return tr("Only numbers are allowed.");
    }
    return {};
}

QtSoftwareKeyboard::QtSoftwareKeyboard(GMainWindow& main_window) {
    connect(this, &QtSoftwareKeyboard::MainWindowInitializeKeyboard, &main_window,
            &GMainWindow::SoftwareKeyboardInitialize, Qt::QueuedConnection);
    connect(this, &QtSoftwareKeyboard::MainWindowShowNormalKeyboard, &main_window,
            &GMainWindow::SoftwareKeyboardShowNormal, Qt::QueuedConnection);
    connect(this, &QtSoftwareKeyboard::MainWindowShowTextCheckDialog, &main_window,
            &GMainWindow::SoftwareKeyboardShowTextCheck, Qt::QueuedConnection);
    connect(this, &QtSoftwareKeyboard::MainWindowShowInlineKeyboard, &main_window,
            &GMainWindow::SoftwareKeyboardShowInline, Qt::QueuedConnection);
    connect(this, &QtSoftwareKeyboard::MainWindowHideInlineKeyboard, &main_window,
            &GMainWindow::SoftwareKeyboardHideInline, Qt::QueuedConnection);
    connect(this, &QtSoftwareKeyboard::MainWindowInlineTextChanged, &main_window,
            &GMainWindow::SoftwareKeyboardInlineTextChanged, Qt::QueuedConnection);
    connect(this, &QtSoftwareKeyboard::MainWindowExitKeyboard, &main_window,
            &GMainWindow::SoftwareKeyboardExit, Qt::QueuedConnection);
    connect(&main_window, &GMainWindow::SoftwareKeyboardSubmitNormalText, this,
            &QtSoftwareKeyboard::SubmitNormalText, Qt::QueuedConnection);
    connect(&main_window, &GMainWindow::SoftwareKeyboardSubmitInlineText, this,
            &QtSoftwareKeyboard::SubmitInlineText, Qt::QueuedConnection);
}

QtSoftwareKeyboard::~QtSoftwareKeyboard() = default;

void QtSoftwareKeyboard::InitializeKeyboard(
    bool is_inline, Core::Frontend::KeyboardInitializeParameters initialize_parameters,
    SubmitNormalCallback submit_normal_callback_, SubmitInlineCallback submit_inline_callback_) {
    if (is_inline) {
        submit_inline_callback = std::move(submit_inline_callback_);
    } else {
        submit_normal_callback = std::move(submit_normal_callback_);
    }

    LOG_INFO(Service_AM,
             "\nKeyboardInitializeParameters:"
             "\nok_text={}"
             "\nheader_text={}"
             "\nsub_text={}"
             "\nguide_text={}"
             "\ninitial_text={}"
             "\nmax_text_length={}"
             "\nmin_text_length={}"
             "\ninitial_cursor_position={}"
             "\ntype={}"
             "\npassword_mode={}"
             "\ntext_draw_type={}"
             "\nkey_disable_flags={}"
             "\nuse_blur_background={}"
             "\nenable_backspace_button={}"
             "\nenable_return_button={}"
             "\ndisable_cancel_button={}",
             Common::UTF16ToUTF8(initialize_parameters.ok_text),
             Common::UTF16ToUTF8(initialize_parameters.header_text),
             Common::UTF16ToUTF8(initialize_parameters.sub_text),
             Common::UTF16ToUTF8(initialize_parameters.guide_text),
             Common::UTF16ToUTF8(initialize_parameters.initial_text),
             initialize_parameters.max_text_length, initialize_parameters.min_text_length,
             initialize_parameters.initial_cursor_position, initialize_parameters.type,
             initialize_parameters.password_mode, initialize_parameters.text_draw_type,
             initialize_parameters.key_disable_flags.raw, initialize_parameters.use_blur_background,
             initialize_parameters.enable_backspace_button,
             initialize_parameters.enable_return_button,
             initialize_parameters.disable_cancel_button);

    emit MainWindowInitializeKeyboard(is_inline, std::move(initialize_parameters));
}

void QtSoftwareKeyboard::ShowNormalKeyboard() const {
    emit MainWindowShowNormalKeyboard();
}

void QtSoftwareKeyboard::ShowTextCheckDialog(
    Service::AM::Frontend::SwkbdTextCheckResult text_check_result,
    std::u16string text_check_message) const {
    emit MainWindowShowTextCheckDialog(text_check_result, std::move(text_check_message));
}

void QtSoftwareKeyboard::ShowInlineKeyboard(
    Core::Frontend::InlineAppearParameters appear_parameters) {
    LOG_INFO(Service_AM,
             "\nInlineAppearParameters:"
             "\nmax_text_length={}"
             "\nmin_text_length={}"
             "\nkey_top_scale_x={}"
             "\nkey_top_scale_y={}"
             "\nkey_top_translate_x={}"
             "\nkey_top_translate_y={}"
             "\ntype={}"
             "\nkey_disable_flags={}"
             "\nkey_top_as_floating={}"
             "\nenable_backspace_button={}"
             "\nenable_return_button={}"
             "\ndisable_cancel_button={}",
             appear_parameters.max_text_length, appear_parameters.min_text_length,
             appear_parameters.key_top_scale_x, appear_parameters.key_top_scale_y,
             appear_parameters.key_top_translate_x, appear_parameters.key_top_translate_y,
             appear_parameters.type, appear_parameters.key_disable_flags.raw,
             appear_parameters.key_top_as_floating, appear_parameters.enable_backspace_button,
             appear_parameters.enable_return_button, appear_parameters.disable_cancel_button);

    emit MainWindowShowInlineKeyboard(std::move(appear_parameters));
}

void QtSoftwareKeyboard::HideInlineKeyboard() const {
    emit MainWindowHideInlineKeyboard();
}

void QtSoftwareKeyboard::InlineTextChanged(Core::Frontend::InlineTextParameters text_parameters) {
    LOG_INFO(Service_AM,
             "\nInlineTextParameters:"
             "\ninput_text={}"
             "\ncursor_position={}",
             Common::UTF16ToUTF8(text_parameters.input_text), text_parameters.cursor_position);

    emit MainWindowInlineTextChanged(std::move(text_parameters));
}

void QtSoftwareKeyboard::ExitKeyboard() const {
    emit MainWindowExitKeyboard();
}

void QtSoftwareKeyboard::SubmitNormalText(Service::AM::Frontend::SwkbdResult result,
                                          std::u16string submitted_text, bool confirmed) const {
    submit_normal_callback(result, submitted_text, confirmed);
}

void QtSoftwareKeyboard::SubmitInlineText(Service::AM::Frontend::SwkbdReplyType reply_type,
                                          std::u16string submitted_text,
                                          s32 cursor_position) const {
    submit_inline_callback(reply_type, submitted_text, cursor_position);
}
