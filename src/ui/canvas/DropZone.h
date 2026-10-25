#pragma once

#include <QStringList>
#include <QWidget>

class QPushButton;

namespace avloop {

// What the canvas shows with no video (spec §5.1): a large drop zone and Open. Files dropped on it are offered to the
// window; the zone itself never opens anything.
class DropZone : public QWidget {
    Q_OBJECT
public:
    explicit DropZone(QWidget* parent = nullptr);

    QPushButton* openButton() const { return open_; }

signals:
    void openRequested();
    void filesDropped(const QStringList& paths);

protected:
    void paintEvent(QPaintEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    QRect zoneRect() const;

    QPushButton* open_ = nullptr;
    bool hovering_ = false;
};

} // namespace avloop
