#pragma once

#include <QImage>
#include <QWidget>

#include <functional>

namespace avloop {

// The Preview's picture (spec §9.1, §9.2): the output frame over a background — Checker (transparency), Black, White,
// Green #00FF00, Navy or an image (an OBS scene behind it) — at Fit, 50%, 100% (pixel-exact) or 200%.
class PreviewView : public QWidget {
    Q_OBJECT
public:
    enum class Background { Checker, Black, White, Green, Navy, Image };

    explicit PreviewView(QWidget* parent = nullptr);
    void setFrame(const QImage& frame);
    void setBackground(Background b, const QImage& image = {});
    void setZoom(double zoom); // 0 = Fit
    QRect frameRect() const;   // where the frame is drawn

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QImage frame_;
    Background background_ = Background::Checker;
    QImage backgroundImage_;
    double zoom_ = 0;
};

// The Preview's slider (spec §9.2): frames 0 … L − 1; the seam at both ends; Crossfade's blended frames as an orange
// strip; Ping-pong's turn points; under it a thin bar of the frames already made.
class PreviewSlider : public QWidget {
    Q_OBJECT
public:
    explicit PreviewSlider(QWidget* parent = nullptr);
    void setLength(int frames);
    void setValue(int k);
    int value() const { return value_; }
    // Crossfade: the strip from `from` to the end; Ping-pong: the turn frames.
    // `seam` false: a clip as cut (Mode None) — no mark where the end joins the start.
    void setSeams(int fadeFrom, const QList<int>& turns, bool seam = true);
    void setCached(std::function<bool(int)> cached);
    double xOf(int k) const;

signals:
    void moved(int k);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    int frameAt(double x) const;

    int length_ = 1;
    int value_ = 0;
    int fadeFrom_ = -1;
    QList<int> turns_;
    bool seam_ = true;
    std::function<bool(int)> cached_;
};

} // namespace avloop
