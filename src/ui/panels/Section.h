#pragma once

#include <QList>
#include <QString>
#include <QWidget>

#include <functional>

class QGridLayout;
class QToolButton;

namespace avloop {

// A collapsible section of the side panel (spec §5.1): a header with a chevron and the title, a body of label/value
// rows (86 px labels, 6 px gaps). Its open/closed state is remembered under ui/sectionOpen/<key>.
class Section : public QWidget {
    Q_OBJECT
public:
    Section(const QString& key, const QString& title, QWidget* parent = nullptr);

    QWidget* body() const { return body_; }
    QGridLayout* grid() const { return grid_; }
    // Appends a row: a quiet label and a value widget.
    void addRow(const QString& label, QWidget* value);
    // Appends a widget across both columns.
    void addWide(QWidget* widget);
    // A row's value and its label: what a setting that does not apply hides together.
    QList<QWidget*> row(QWidget* value) const;
    // The right-click menu's resets (D-025): "Reset <title>" and any others, each named "menu.reset.<key>" for walks.
    void addResetAction(const QString& text, const QString& key, std::function<void()> reset);

    bool isOpen() const;
    void setOpen(bool open);

signals:
    void toggled(bool open);

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    void updateHeader();

    QString key_;
    struct ResetAction {
        QString text;
        QString key;
        std::function<void()> reset;
    };
    QList<ResetAction> resets_;
    QToolButton* header_ = nullptr;
    QWidget* body_ = nullptr;
    QGridLayout* grid_ = nullptr;
};

} // namespace avloop
