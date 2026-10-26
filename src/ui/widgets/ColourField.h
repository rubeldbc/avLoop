#pragma once

#include <QColor>
#include <QWidget>

#include <functional>

class QLineEdit;
class QToolButton;

namespace avloop {

// A colour in a panel row (D-018): its hex typed ("#rrggbb", taken on Enter or when the focus leaves), a swatch that
// opens the colour dialog, and — when a picker is given — an eyedropper that takes it from the canvas. The hex field
// carries the field's name (walks type into it), the swatch "<name>.choose", the eyedropper "<name>.pick".
class ColourField : public QWidget {
    Q_OBJECT
public:
    // Asks for a colour from the canvas; `done` receives it (never, when the pick is cancelled).
    using Picker = std::function<void(std::function<void(const QColor&)> done)>;

    ColourField(const QString& name, const QString& title, QWidget* parent = nullptr);
    void setColourQuietly(const QColor& colour); // a change that came from the model
    QColor colour() const { return colour_; }
    void setPicker(Picker pick);

signals:
    void colourChosen(const QColor& colour);

private:
    void take(const QColor& colour);
    void display(const QColor& colour);

    QString title_;
    QColor colour_;
    QLineEdit* hex_ = nullptr;
    QToolButton* swatch_ = nullptr;
    QToolButton* pick_ = nullptr;
    Picker picker_;
};

} // namespace avloop
