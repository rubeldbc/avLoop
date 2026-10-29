#pragma once

#include <QStringList>
#include <QWidget>

class QComboBox;
class QListWidget;
class QPushButton;

namespace avloop {

class Project;

// The Project panel (D-033): the media bin, as Premiere's — Import… adds files (probed; the bin is not in Undo), the
// list shows each one's name, size and length; Insert and Overwrite put the selected one at the playhead on the target
// track (V1 … V8), centred in the frame at 100 %: Insert cuts what is there and pushes the rest of that track right,
// Overwrite covers it. One Undo step each.
class ProjectPanel : public QWidget {
    Q_OBJECT
public:
    explicit ProjectPanel(Project* project, QWidget* parent = nullptr);

    // Probes and adds `paths` to the bin (a file that is not a video is said and skipped); the last added selected.
    void importFiles(const QStringList& paths);
    // The selected media placed at the playhead (Overwrite or Insert); false (said) when refused.
    bool place(bool overwrite);

signals:
    void warn(const QString& text);

private:
    void refresh();

    Project* project_;
    QListWidget* list_ = nullptr;
    QComboBox* track_ = nullptr;
    QPushButton* insert_ = nullptr;
    QPushButton* overwrite_ = nullptr;
};

} // namespace avloop
