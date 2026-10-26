// The crop tools (M Rect, U Rounded, E Ellipse; spec §6.1). With no crop a drag draws one; with a crop a drag inside
// moves it, the eight handles resize it, a drag outside draws a new one that replaces it (one Undo step). Shift locks
// the ratio (1:1 when Free, a circle for the Ellipse), Alt works from the centre, Ctrl turns snapping off; arrows move
// it 1 px (Shift 10), Esc cancels a drawing, Delete removes the crop, a double-click makes it the whole video.
#include "core/AspectRatio.h"
#include "core/Geometry.h"
#include "core/Project.h"
#include "ui/canvas/Canvas.h"
#include "ui/options/OptionsBar.h"
#include "ui/theme/Theme.h"
#include "ui/tools/CropHandles.h"
#include "ui/tools/Tool.h"

#include <QKeyEvent>
#include <QPainter>

#include <cmath>
#include <optional>

namespace avloop {

namespace {

using crophandles::Handle;
constexpr double kHandle = 8; // px
constexpr double kSnapPx = 8; // view px (spec §6.1)

class CropTool : public Tool {
public:
    CropTool(CropShape::Kind kind, Canvas* canvas, Project* project)
        : Tool(CropShape::toolId(kind), canvas, project), kind_(kind) {}

    void activate() override {
        // Switching crop tools changes the crop's kind; its rectangle stays (spec §6.1).
        CropShape c = project_->crop();
        if (!c.isEmpty() && c.kind != kind_) {
            c.kind = kind_;
            project_->changeCrop(c);
        }
    }

    void mousePress(const ToolEvent& e) override {
        if (e.button != Qt::LeftButton || !project_->source().isValid()) return;
        const CropShape& c = project_->crop();
        startRect_ = c.rect;
        startPointer_ = e.scene;
        mergeKey_ = QStringLiteral("drag:crop:%1").arg(++drags_);
        handle_ = c.isEmpty() ? Handle::None : crophandles::hit(canvas_->sceneToView(c.rect), e.view);
        if (handle_ != Handle::None)
            mode_ = Mode::Resize;
        else if (!c.isEmpty() && c.rect.contains(e.scene))
            mode_ = Mode::Move;
        else {
            mode_ = Mode::Draw;
            anchor_ = snapPoint(e.scene, e.modifiers);
            preview_.reset();
        }
    }

    void mouseMove(const ToolEvent& e) override {
        if (mode_ == Mode::None) return;
        snapLines_.clear();
        const double ratio = ratioFor(e.modifiers);
        const bool fromCentre = e.modifiers & Qt::AltModifier;
        if (mode_ == Mode::Draw) {
            CropShape c = project_->crop();
            c.kind = kind_;
            c.rect = crophandles::drawn(anchor_, snapPoint(e.scene, e.modifiers), ratio, fromCentre);
            preview_ = c;
            return;
        }
        CropShape c = project_->crop();
        const QPointF delta = e.scene - startPointer_;
        if (mode_ == Mode::Move)
            c.rect = snapMove(startRect_.translated(delta), e.modifiers);
        else
            c.rect = crophandles::resized(startRect_, handle_, snapDelta(delta, e.modifiers), ratio, fromCentre);
        project_->changeCrop(c, mergeKey_);
    }

    void mouseRelease(const ToolEvent&) override {
        if (mode_ == Mode::Draw && preview_) {
            // A drawing under 16 px is cancelled (spec §12.2); otherwise it replaces the crop in one step.
            if (preview_->rect.width() >= CropShape::kMinSide && preview_->rect.height() >= CropShape::kMinSide)
                project_->changeCrop(*preview_);
        }
        preview_.reset();
        snapLines_.clear();
        mode_ = Mode::None;
    }

    void mouseDoubleClick(const ToolEvent&) override {
        CropShape c = project_->crop();
        c.kind = kind_;
        c.rect = project_->transform().rect(project_->source().size());
        project_->changeCrop(c);
    }

    bool keyPress(QKeyEvent* e) override {
        CropShape c = project_->crop();
        if (e->key() == Qt::Key_Escape && mode_ == Mode::Draw) {
            preview_.reset();
            mode_ = Mode::None;
            return true;
        }
        if (c.isEmpty()) return false;
        if (e->key() == Qt::Key_Delete) {
            c.rect = QRectF();
            project_->changeCrop(c);
            return true;
        }
        const double step = (e->modifiers() & Qt::ShiftModifier) ? 10 : 1;
        QPointF d;
        if (e->key() == Qt::Key_Left)
            d = {-step, 0};
        else if (e->key() == Qt::Key_Right)
            d = {step, 0};
        else if (e->key() == Qt::Key_Up)
            d = {0, -step};
        else if (e->key() == Qt::Key_Down)
            d = {0, step};
        else
            return false;
        c.rect.translate(d);
        project_->changeCrop(c, QStringLiteral("nudge"));
        return true;
    }

    const CropShape* previewCrop() const override { return preview_ ? &*preview_ : nullptr; }

    Qt::CursorShape cursor(const ToolEvent& e) const override {
        const CropShape& c = project_->crop();
        if (c.isEmpty()) return Qt::CrossCursor;
        const Handle h = crophandles::hit(canvas_->sceneToView(c.rect), e.view);
        if (h != Handle::None) return crophandles::cursor(h);
        return c.rect.contains(e.scene) ? Qt::SizeAllCursor : Qt::CrossCursor;
    }

    void paintOverlay(QPainter& p) override {
        const ThemeTokens& t = Theme::instance().tokens();
        const CropShape c = preview_ ? *preview_ : project_->crop();
        if (c.isEmpty()) return;
        const QRectF r = canvas_->sceneToView(c.rect);
        if (mode_ != Mode::None) {
            // Rule-of-thirds guides and the size label while dragging (spec §6.1).
            p.setPen(QPen(QColor(255, 255, 255, 64), 1));
            for (int i = 1; i < 3; ++i) {
                p.drawLine(QPointF(r.left() + r.width() * i / 3, r.top()),
                           QPointF(r.left() + r.width() * i / 3, r.bottom()));
                p.drawLine(QPointF(r.left(), r.top() + r.height() * i / 3),
                           QPointF(r.right(), r.top() + r.height() * i / 3));
            }
            const QSize out = geometry::outputSize(c.rect.size());
            const QString label = QStringLiteral("%1 × %2  →  %3 × %4")
                                      .arg(std::lround(c.rect.width()))
                                      .arg(std::lround(c.rect.height()))
                                      .arg(out.width())
                                      .arg(out.height());
            QFont mono(QStringLiteral("Consolas"), 9);
            p.setFont(mono);
            const QRectF box(r.left(), r.bottom() + 6, QFontMetricsF(mono).horizontalAdvance(label) + 12, 18);
            QColor back = t.bg;
            back.setAlpha(230);
            p.fillRect(box, back);
            p.setPen(t.accentHover);
            p.drawText(box, Qt::AlignCenter, label);
            QPen snapPen(t.text, 1, Qt::DashLine);
            p.setPen(snapPen);
            for (const QLineF& l : snapLines_)
                p.drawLine(QLineF(canvas_->sceneToView(l.p1()), canvas_->sceneToView(l.p2())));
        }
        p.setPen(QPen(t.accent, 1.5));
        p.setBrush(Qt::white);
        for (Handle h : crophandles::all()) {
            const QPointF at = crophandles::position(r, h);
            p.drawRect(QRectF(at.x() - kHandle / 2, at.y() - kHandle / 2, kHandle, kHandle));
        }
    }

    QWidget* createOptions(QWidget* parent) override {
        return createCropOptions(project_, canvas_, int(kind_), parent);
    }

private:
    enum class Mode { None, Draw, Move, Resize };

    // The ratio a drag keeps: the crop's own; with none, Shift's 1:1 (a circle for the Ellipse).
    double ratioFor(Qt::KeyboardModifiers mods) const {
        const double r = aspect::value(project_->crop().ratio, project_->source().size());
        if (r > 0) return r;
        return (mods & Qt::ShiftModifier) ? 1.0 : 0.0;
    }

    QRectF videoRect() const { return project_->transform().rect(project_->source().size()); }
    double tolerance() const { return kSnapPx / canvas_->zoom(); }

    QPointF snapPoint(const QPointF& p, Qt::KeyboardModifiers mods) {
        if (mods & Qt::ControlModifier) return p;
        const QRectF v = videoRect();
        double lx = NAN, ly = NAN;
        const QPointF s(crophandles::snap(p.x(), crophandles::targetsX(v), tolerance(), &lx),
                        crophandles::snap(p.y(), crophandles::targetsY(v), tolerance(), &ly));
        if (!std::isnan(lx)) snapLines_.append(QLineF(lx, v.top() - 1e4, lx, v.bottom() + 1e4));
        if (!std::isnan(ly)) snapLines_.append(QLineF(v.left() - 1e4, ly, v.right() + 1e4, ly));
        return s;
    }

    // A moved crop: its left, centre or right (top, centre, bottom) to the video's nearest edge or centre.
    QRectF snapMove(const QRectF& r, Qt::KeyboardModifiers mods) {
        if (mods & Qt::ControlModifier) return r;
        const QRectF v = videoRect();
        // The shift that brings the nearest of a, c, b onto a target, 0 when none is within the tolerance.
        auto axis = [&](double a, double c, double b, const QList<double>& targets) {
            double best = 0;
            double bestD = tolerance();
            for (double value : {a, c, b})
                for (double t : targets)
                    if (std::abs(t - value) <= bestD) {
                        bestD = std::abs(t - value);
                        best = t - value;
                    }
            return best;
        };
        const double dx = axis(r.left(), r.center().x(), r.right(), crophandles::targetsX(v));
        const double dy = axis(r.top(), r.center().y(), r.bottom(), crophandles::targetsY(v));
        return r.translated(dx, dy);
    }

    // A resize: the moving edges snap; the drag's delta is corrected by the edge's snap.
    QPointF snapDelta(const QPointF& delta, Qt::KeyboardModifiers mods) {
        if (mods & Qt::ControlModifier) return delta;
        const QRectF v = videoRect();
        const QRectF moved = crophandles::resized(startRect_, handle_, delta, 0, false);
        QPointF d = delta;
        auto fix = [&](double edge, const QList<double>& targets, double& out) {
            out += crophandles::snap(edge, targets, tolerance()) - edge;
        };
        if (moved.left() != startRect_.left())
            fix(moved.left(), crophandles::targetsX(v), d.rx());
        else if (moved.right() != startRect_.right())
            fix(moved.right(), crophandles::targetsX(v), d.rx());
        if (moved.top() != startRect_.top())
            fix(moved.top(), crophandles::targetsY(v), d.ry());
        else if (moved.bottom() != startRect_.bottom())
            fix(moved.bottom(), crophandles::targetsY(v), d.ry());
        return d;
    }

    CropShape::Kind kind_;
    Mode mode_ = Mode::None;
    Handle handle_ = Handle::None;
    QRectF startRect_;
    QPointF startPointer_;
    QPointF anchor_;
    std::optional<CropShape> preview_;
    QList<QLineF> snapLines_;
    QString mergeKey_;
    int drags_ = 0;
};

} // namespace

Tool* createCropTool(int kind, Canvas* canvas, Project* project) {
    return new CropTool(static_cast<CropShape::Kind>(kind), canvas, project);
}

} // namespace avloop
