#pragma once

#include <QFrame>
#include <QString>

#include <functional>

class QLabel;
class QPushButton;

namespace avloop {

// A short message in the host's bottom-right corner (spec §3.6, §10.1): it replaces a toast still shown there,
// hides itself after its time, and may carry one action button ("Show in folder", "Don't show again").
class Toast : public QFrame {
    Q_OBJECT
public:
    enum class Kind { Info, Success, Warning, Error };

    static Toast* post(QWidget* host, const QString& text, Kind kind = Kind::Info, int ms = 5000,
                       const QString& actionText = {}, std::function<void()> action = {});
    static Toast* current(QWidget* host);

    QString text() const;
    Kind kind() const { return kind_; }
    void dismiss();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    Toast(QWidget* host, const QString& text, Kind kind);
    void place();

    QLabel* label_ = nullptr;
    QPushButton* button_ = nullptr;
    Kind kind_;
};

} // namespace avloop
