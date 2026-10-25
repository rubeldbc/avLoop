#pragma once

#include <QString>
#include <QWidget>

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

    bool isOpen() const;
    void setOpen(bool open);

signals:
    void toggled(bool open);

private:
    void updateHeader();

    QString key_;
    QToolButton* header_ = nullptr;
    QWidget* body_ = nullptr;
    QGridLayout* grid_ = nullptr;
};

} // namespace avloop
