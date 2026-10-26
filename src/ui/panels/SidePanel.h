#pragma once

#include <QWidget>

class QComboBox;
class QToolButton;

namespace avloop {

class Project;
class Section;

// The right panel (spec §5.1): the preset row above four collapsible sections — Crop, Loop, Output, Quality.
// Each later phase fills its section's rows.
class SidePanel : public QWidget {
    Q_OBJECT
public:
    explicit SidePanel(QWidget* parent = nullptr);

    Section* crop() const { return crop_; }
    Section* loop() const { return loop_; }
    Section* output() const { return output_; }
    Section* quality() const { return quality_; }
    QComboBox* presetBox() const { return presets_; }
    // CropPanel.cpp: the Crop section's rows, bound to the project.
    void bindCrop(Project* project);
    // LoopPanel.cpp: the Loop section; OutputPanel.cpp: the Output section (its frame rate until AV-009).
    void bindLoop(Project* project);
    void bindOutput(Project* project);

private:
    QComboBox* presets_ = nullptr;
    QToolButton* savePreset_ = nullptr;
    QToolButton* deletePreset_ = nullptr;
    Section* crop_ = nullptr;
    Section* loop_ = nullptr;
    Section* output_ = nullptr;
    Section* quality_ = nullptr;
};

} // namespace avloop
