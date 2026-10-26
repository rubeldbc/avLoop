#include "ui/preview/PreviewWidgets.h"

#include "ui/theme/Theme.h"

#include <QMouseEvent>
#include <QPainter>

#include <algorithm>
#include <cmath>

namespace avloop {

namespace {
constexpr int kEnds = 10; // px either side of the slider's track
}

PreviewView::PreviewView(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("preview.view"));
    setMinimumSize(160, 120);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void PreviewView::setFrame(const QImage& frame) {
    frame_ = frame;
    update();
}

void PreviewView::setBackground(Background b, const QImage& image) {
    background_ = b;
    backgroundImage_ = image;
    update();
}

void PreviewView::setZoom(double zoom) {
    zoom_ = zoom;
    update();
}

QRect PreviewView::frameRect() const {
    if (frame_.isNull()) return {};
    QSizeF s = frame_.size();
    if (zoom_ <= 0)
        s = s.scaled(QSizeF(size()) - QSizeF(16, 16), Qt::KeepAspectRatio); // Fit
    else
        s *= zoom_;
    const QSize whole(int(std::lround(s.width())), int(std::lround(s.height())));
    return QRect(QPoint((width() - whole.width()) / 2, (height() - whole.height()) / 2), whole);
}

void PreviewView::paintEvent(QPaintEvent*) {
    QPainter p(this);
    const ThemeTokens& t = Theme::instance().tokens();
    p.fillRect(rect(), t.bg);
    const QRect r = frameRect();
    if (r.isEmpty()) return;
    switch (background_) {
    case Background::Checker: {
        // 8 px cells, the transparency the canvas shows.
        const QColor a(0x40, 0x44, 0x4c), b(0x5a, 0x5e, 0x66);
        p.fillRect(r, a);
        for (int y = r.top(); y < r.bottom(); y += 8)
            for (int x = r.left() + ((y - r.top()) / 8 % 2) * 8; x < r.right(); x += 16)
                p.fillRect(QRect(x, y, 8, 8).intersected(r), b);
        break;
    }
    case Background::Black:
        p.fillRect(r, Qt::black);
        break;
    case Background::White:
        p.fillRect(r, Qt::white);
        break;
    case Background::Green:
        p.fillRect(r, QColor(0x00, 0xFF, 0x00));
        break;
    case Background::Navy:
        p.fillRect(r, QColor(0x10, 0x1a, 0x33));
        break;
    case Background::Image:
        if (backgroundImage_.isNull())
            p.fillRect(r, Qt::black);
        else
            p.drawImage(r, backgroundImage_.scaled(r.size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation),
                        QRect(QPoint(0, 0), r.size()));
        break;
    }
    // Pixel-exact at whole zooms; smooth when Fit or 50% scales it.
    const bool whole = zoom_ >= 1 && std::abs(zoom_ - std::round(zoom_)) < 1e-9;
    p.setRenderHint(QPainter::SmoothPixmapTransform, !whole);
    p.drawImage(r, frame_);
}

PreviewSlider::PreviewSlider(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("preview.slider"));
    setMinimumHeight(30);
    setMouseTracking(false);
}

void PreviewSlider::setLength(int frames) {
    length_ = std::max(1, frames);
    value_ = std::clamp(value_, 0, length_ - 1);
    update();
}

void PreviewSlider::setValue(int k) {
    value_ = std::clamp(k, 0, length_ - 1);
    update();
}

void PreviewSlider::setSeams(int fadeFrom, const QList<int>& turns) {
    fadeFrom_ = fadeFrom;
    turns_ = turns;
    update();
}

void PreviewSlider::setCached(std::function<bool(int)> cached) {
    cached_ = std::move(cached);
    update();
}

double PreviewSlider::xOf(int k) const {
    const double span = std::max(1, width() - 2 * kEnds);
    return kEnds + (length_ <= 1 ? 0.0 : span * k / (length_ - 1));
}

int PreviewSlider::frameAt(double x) const {
    const double span = std::max(1, width() - 2 * kEnds);
    return std::clamp(int(std::lround((x - kEnds) / span * (length_ - 1))), 0, length_ - 1);
}

void PreviewSlider::paintEvent(QPaintEvent*) {
    QPainter p(this);
    const ThemeTokens& t = Theme::instance().tokens();
    const int mid = height() / 2 - 3;
    const double x0 = xOf(0), x1 = xOf(length_ - 1);
    p.fillRect(QRectF(x0, mid - 2, x1 - x0, 4), t.border);
    // Crossfade: the frames that hold the blend.
    if (fadeFrom_ >= 0 && fadeFrom_ < length_) {
        QColor strip = t.accent;
        strip.setAlpha(150);
        p.fillRect(QRectF(QPointF(xOf(fadeFrom_), mid - 3), QPointF(x1, mid + 3)), strip);
    }
    // The seam: where the end joins the start; the Ping-pongs' turns.
    p.setPen(QPen(t.accent, 2));
    for (double x : {x0, x1})
        p.drawLine(QPointF(x, mid - 7), QPointF(x, mid + 7));
    for (int k : turns_)
        p.drawLine(QPointF(xOf(k), mid + 4), QPointF(xOf(k), mid + 9));
    // The frames already made: a thin bar under the track.
    if (cached_) {
        p.setPen(Qt::NoPen);
        QColor made = t.text2;
        made.setAlpha(170);
        for (int k = 0; k < length_;) {
            if (!cached_(k)) {
                ++k;
                continue;
            }
            const int from = k;
            while (k < length_ && cached_(k))
                ++k;
            p.fillRect(QRectF(QPointF(xOf(from) - 0.5, height() - 4), QPointF(xOf(k - 1) + 0.5, height() - 2)), made);
        }
    }
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(t.text);
    p.setPen(QPen(t.accent, 2));
    p.drawEllipse(QPointF(xOf(value_), mid), 6, 6);
}

void PreviewSlider::mousePressEvent(QMouseEvent* event) {
    value_ = frameAt(event->position().x());
    update();
    emit moved(value_);
}

void PreviewSlider::mouseMoveEvent(QMouseEvent* event) {
    if (!(event->buttons() & Qt::LeftButton)) return;
    const int k = frameAt(event->position().x());
    if (k == value_) return;
    value_ = k;
    update();
    emit moved(k);
}

} // namespace avloop
