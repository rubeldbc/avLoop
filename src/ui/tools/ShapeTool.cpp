// The Shape tool (S; D-022): shapes inside the crop's frame that select or cut. Its bar chooses what is drawn — a
// rectangle, a rounded rectangle or an ellipse by a drag (Shift square or circle, Alt from the centre), a polygon by
// clicks (a click on its first anchor, Enter or a double-click closes it; Esc cancels), a freehand outline by a drag
// (simplified to 1 scene px) — and the role a new shape takes. A click selects the topmost shape under it (Shift adds
// to the selection), a drag moves the selected; a single standard shape has the crop's eight handles, a custom one its
// anchors; Delete removes, arrows nudge (Shift 10 px), Esc deselects. Each edit is one Undo step of the crop.
#include "core/Project.h"
#include "ui/WalkRunner.h"
#include "ui/canvas/Canvas.h"
#include "ui/options/OptionsBar.h"
#include "ui/theme/Theme.h"
#include "ui/tools/CropHandles.h"
#include "ui/tools/ShapeEdit.h"
#include "ui/tools/Tool.h"

#include <QKeyEvent>
#include <QLineF>
#include <QPainter>
#include <QPainterPath>

namespace avloop {

namespace {

using crophandles::Handle;
constexpr double kHandle = 8; // px
constexpr double kClose = 8;  // view px: a click this near the first anchor closes a polygon

class ShapeTool : public Tool {
public:
    ShapeTool(Canvas* canvas, Project* project) : Tool(QStringLiteral("shape"), canvas, project) {
        WalkProbes::add(QStringLiteral("shape.mode"), [this] { return modeName(); });
        WalkProbes::add(QStringLiteral("shape.points"), [this] { return int(points_.size()); });
        // The last freehand outline: its points as dragged and as kept ("raw/kept").
        WalkProbes::add(QStringLiteral("shape.freehand"), [this] { return freehand_; });
    }

    void deactivate() override { cancel(); }

    void mousePress(const ToolEvent& e) override {
        if (e.button != Qt::LeftButton || !project_->source().isValid()) return;
        const QList<MaskShape>& shapes = project_->crop().shapes;
        start_ = e.scene;
        mergeKey_ = QStringLiteral("drag:shape:%1").arg(++drags_);
        if (mode_ == Mode::Polygon) {
            addPolygonPoint(e);
            return;
        }
        const QList<int> selected = project_->selectedShapes();
        // One selected shape: a standard one's handle resizes it, a custom one's anchor moves alone.
        if (selected.size() == 1 && !shapes[selected.first()].locked) {
            const MaskShape& s = shapes[selected.first()];
            index_ = selected.first();
            startShape_ = s;
            if (s.isStandard()) {
                handle_ = crophandles::hit(canvas_->sceneToView(s.rect), e.view);
                if (handle_ != Handle::None) {
                    mode_ = Mode::Resize;
                    return;
                }
            } else if (anchor_ = shapeedit::hitAnchor(s, canvas_->sceneToViewTransform(), e.view); anchor_ >= 0) {
                mode_ = Mode::Anchor;
                return;
            }
        }
        if (const int hit = shapeedit::hitShape(shapes, e.scene); hit >= 0) {
            QList<int> next = selected;
            if (e.modifiers & Qt::ShiftModifier) {
                if (!next.contains(hit)) next.append(hit);
            } else if (!selected.contains(hit)) {
                next = {hit};
            }
            project_->setSelectedShapes(next);
            mode_ = Mode::Move;
            startShapes_ = shapes;
            moving_ = project_->selectedShapes();
            return;
        }
        project_->setSelectedShapes({});
        points_ = {e.scene};
        hover_ = e.scene;
        rect_ = QRectF();
        mode_ = draft_.kind == ShapeDraft::Kind::Polygon    ? Mode::Polygon
                : draft_.kind == ShapeDraft::Kind::Freehand ? Mode::Freehand
                                                            : Mode::Draw;
    }

    void mouseMove(const ToolEvent& e) override {
        const QPointF d = e.scene - start_;
        CropShape c = project_->crop();
        switch (mode_) {
        case Mode::Draw:
            rect_ = crophandles::drawn(start_, e.scene, (e.modifiers & Qt::ShiftModifier) ? 1.0 : 0.0,
                                       e.modifiers & Qt::AltModifier);
            break;
        case Mode::Freehand:
            if (QLineF(points_.last(), e.scene).length() * canvas_->zoom() >= 2) points_.append(e.scene);
            break;
        case Mode::Polygon:
            hover_ = e.scene;
            break;
        case Mode::Move:
            c.shapes = shapeedit::moved(startShapes_, moving_, d);
            project_->changeCrop(c, mergeKey_);
            break;
        case Mode::Resize:
            if (index_ < c.shapes.size()) {
                const QRectF r = startShape_.rect;
                const double ratio = (e.modifiers & Qt::ShiftModifier) && r.height() > 0 ? r.width() / r.height() : 0;
                MaskShape s = startShape_;
                s.rect = crophandles::resized(r, handle_, d, ratio, e.modifiers & Qt::AltModifier);
                c.shapes[index_] = s.clamped();
                project_->changeCrop(c, mergeKey_);
            }
            break;
        case Mode::Anchor:
            if (index_ < c.shapes.size() && anchor_ < startShape_.points.size()) {
                MaskShape s = startShape_;
                PathPoint& pt = s.points[anchor_];
                pt.p += d;
                pt.in += d;
                pt.out += d;
                c.shapes[index_] = s;
                project_->changeCrop(c, mergeKey_);
            }
            break;
        case Mode::None:
            break;
        }
    }

    void mouseRelease(const ToolEvent&) override {
        if (mode_ == Mode::Draw) {
            if (rect_.width() >= shapeedit::kMinSide && rect_.height() >= shapeedit::kMinSide)
                add(shapeedit::standard(draft_.kind, draft_.role, rect_));
        } else if (mode_ == Mode::Freehand) {
            const QList<QPointF> pts = maskshapes::simplified(points_, 1.0);
            freehand_ = QStringLiteral("%1/%2").arg(points_.size()).arg(pts.size());
            QPainterPath outline;
            outline.addPolygon(QPolygonF(pts));
            const QRectF box = outline.boundingRect();
            if (pts.size() >= 3 && std::max(box.width(), box.height()) >= shapeedit::kMinSide)
                add(shapeedit::polygon(pts, draft_.role));
        }
        if (mode_ != Mode::Polygon) cancel();
    }

    void mouseDoubleClick(const ToolEvent&) override {
        if (mode_ != Mode::Polygon) return;
        // The double-click's first press added an anchor where the second one stands: it is not another.
        if (points_.size() >= 2 && QLineF(points_.last(), points_[points_.size() - 2]).length() * canvas_->zoom() < 3)
            points_.removeLast();
        closePolygon();
    }

    bool keyPress(QKeyEvent* e) override {
        const QList<int> selected = project_->selectedShapes();
        if (e->key() == Qt::Key_Escape) {
            if (mode_ != Mode::None)
                cancel();
            else
                project_->setSelectedShapes({});
            return true;
        }
        if ((e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) && mode_ == Mode::Polygon) {
            closePolygon();
            return true;
        }
        if (selected.isEmpty()) return false;
        CropShape c = project_->crop();
        if (e->key() == Qt::Key_Delete || e->key() == Qt::Key_Backspace) {
            c.shapes = shapeedit::removed(c.shapes, selected);
            project_->setSelectedShapes({});
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
        c.shapes = shapeedit::moved(c.shapes, selected, d);
        project_->changeCrop(c, QStringLiteral("nudge:shapes"));
        return true;
    }

    bool wantsKey(const QKeyEvent* e) const override {
        const bool selection = !project_->selectedShapes().isEmpty();
        switch (e->key()) {
        case Qt::Key_Escape:
            return mode_ != Mode::None || selection;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            return mode_ == Mode::Polygon;
        case Qt::Key_Delete:
        case Qt::Key_Backspace:
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Up:
        case Qt::Key_Down:
            return selection;
        default:
            return false;
        }
    }

    Qt::CursorShape cursor(const ToolEvent& e) const override {
        hover_ = e.scene; // the canvas asks for the cursor on a move without a button: a polygon's next segment follows
        const QList<MaskShape>& shapes = project_->crop().shapes;
        const QList<int> selected = project_->selectedShapes();
        if (selected.size() == 1 && shapes[selected.first()].isStandard()) {
            const Handle h = crophandles::hit(canvas_->sceneToView(shapes[selected.first()].rect), e.view);
            if (h != Handle::None) return crophandles::cursor(h);
        }
        return mode_ == Mode::None && shapeedit::hitShape(shapes, e.scene) >= 0 ? Qt::SizeAllCursor : Qt::CrossCursor;
    }

    void paintOverlay(QPainter& p) override {
        const ThemeTokens& t = Theme::instance().tokens();
        const QTransform toView = canvas_->sceneToViewTransform();
        QPen dashed(t.accentHover, 1.25, Qt::DashLine);
        p.setBrush(Qt::NoBrush);
        // What is being drawn.
        if (mode_ == Mode::Draw && !rect_.isEmpty()) {
            p.setPen(dashed);
            p.drawPath(toView.map(shapeedit::standard(draft_.kind, draft_.role, rect_).path()));
        } else if (mode_ == Mode::Freehand || mode_ == Mode::Polygon) {
            QPolygonF line;
            for (const QPointF& pt : points_)
                line << toView.map(pt);
            if (mode_ == Mode::Polygon) line << toView.map(hover_);
            p.setPen(dashed);
            p.drawPolyline(line);
            if (mode_ == Mode::Polygon) anchors(p, QPolygonF(line.mid(0, line.size() - 1)), 0);
        }
        // The selection: one standard shape's eight handles; a custom one's anchors; several, their boxes.
        const QList<MaskShape>& shapes = project_->crop().shapes;
        const QList<int> selected = project_->selectedShapes();
        for (int i : selected) {
            const MaskShape& s = shapes[i];
            p.setPen(QPen(t.text, 1, Qt::DotLine));
            p.setBrush(Qt::NoBrush);
            p.drawRect(toView.mapRect(s.bounds()));
            if (selected.size() > 1 || s.locked) continue;
            if (s.isStandard()) {
                p.setPen(QPen(t.accent, 1.5));
                p.setBrush(Qt::white);
                const QRectF r = canvas_->sceneToView(s.rect);
                for (Handle h : crophandles::all()) {
                    const QPointF at = crophandles::position(r, h);
                    p.drawRect(QRectF(at.x() - kHandle / 2, at.y() - kHandle / 2, kHandle, kHandle));
                }
            } else {
                QPolygonF at;
                for (const PathPoint& pt : s.points)
                    at << toView.map(pt.p);
                anchors(p, at, -1);
            }
        }
    }

    QWidget* createOptions(QWidget* parent) override { return createShapeOptions(project_, &draft_, parent); }

private:
    enum class Mode { None, Draw, Freehand, Polygon, Move, Resize, Anchor };

    QString modeName() const {
        static const char* names[] = {"none", "draw", "freehand", "polygon", "move", "resize", "anchor"};
        return QString::fromLatin1(names[int(mode_)]);
    }

    void anchors(QPainter& p, const QPolygonF& at, int highlight) {
        const ThemeTokens& t = Theme::instance().tokens();
        p.setPen(QPen(t.accent, 1.25));
        for (qsizetype i = 0; i < at.size(); ++i) {
            p.setBrush(i == highlight ? t.accent : QColor(Qt::white));
            p.drawRect(QRectF(at[i].x() - 3.5, at[i].y() - 3.5, 7, 7));
        }
    }

    void addPolygonPoint(const ToolEvent& e) {
        const QPointF first = canvas_->sceneToView(points_.first());
        if (points_.size() >= 3 && QLineF(first, e.view).length() <= kClose) {
            closePolygon();
            return;
        }
        points_.append(e.scene);
    }

    void closePolygon() {
        if (points_.size() >= 3) add(shapeedit::polygon(points_, draft_.role));
        cancel();
    }

    void add(MaskShape s) {
        CropShape c = project_->crop();
        if (c.shapes.size() >= MaskShape::kMaxShapes) return;
        s.name = maskshapes::nextName(c.shapes, s.kind);
        c.shapes.append(s);
        project_->changeCrop(c);
        project_->setSelectedShapes({int(c.shapes.size() - 1)});
    }

    void cancel() {
        mode_ = Mode::None;
        points_.clear();
        rect_ = QRectF();
    }

    ShapeDraft draft_;
    Mode mode_ = Mode::None;
    Handle handle_ = Handle::None;
    QPointF start_;
    mutable QPointF hover_;
    QRectF rect_;
    QList<QPointF> points_;
    QList<MaskShape> startShapes_;
    QList<int> moving_;
    MaskShape startShape_;
    qsizetype index_ = 0;
    int anchor_ = -1;
    QString mergeKey_;
    QString freehand_;
    int drags_ = 0;
};

} // namespace

Tool* createShapeTool(Canvas* canvas, Project* project) {
    return new ShapeTool(canvas, project);
}

} // namespace avloop
