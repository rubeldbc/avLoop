// The Pen (P; D-022, AV-025), as Illustrator's: a click adds a corner anchor, a click-and-drag a smooth one with its
// two handles (mirrored); a click on the first anchor or Enter closes the path (one Undo step), Esc drops it. On the
// selected path (or a polygon, which becomes a path): a handle dragged turns its pair together, Alt breaks the pair; an
// anchor dragged moves it with its handles, Alt+click converts it smooth ↔ corner; a click on a segment adds an anchor
// there, the outline unchanged; Delete removes the selected anchor (or the path, two anchors left). Ctrl+click is the
// direct selection: it selects the shape under it without drawing. Each edit is one Undo step of the crop.
#include "core/Project.h"
#include "ui/WalkRunner.h"
#include "ui/canvas/Canvas.h"
#include "ui/options/OptionsBar.h"
#include "ui/theme/Theme.h"
#include "ui/tools/PathEdit.h"
#include "ui/tools/ShapeEdit.h"
#include "ui/tools/Tool.h"

#include <QKeyEvent>
#include <QLineF>
#include <QPainter>
#include <QPainterPath>

#include <algorithm>

namespace avloop {

namespace {

constexpr double kClose = 8;  // view px: a click this near the first anchor closes the path
constexpr double kDragPx = 3; // view px: a press moved this far makes the new anchor smooth

class PenTool : public Tool {
public:
    PenTool(Canvas* canvas, Project* project) : Tool(QStringLiteral("pen"), canvas, project) {
        WalkProbes::add(QStringLiteral("pen.drawing"), [this] { return int(drawing_.size()); });
        WalkProbes::add(QStringLiteral("pen.anchor"), [this] { return anchor_; });
        WalkProbes::add(QStringLiteral("pen.anchors"), [this] {
            const int i = editable();
            return i >= 0 ? pathedit::kinds(project_->crop().shapes[i]) : QString();
        });
    }

    void deactivate() override {
        drawing_.clear();
        anchor_ = -1;
    }

    void mousePress(const ToolEvent& e) override {
        if (e.button != Qt::LeftButton || !project_->source().isValid()) return;
        start_ = e.scene;
        mergeKey_ = QStringLiteral("drag:pen:%1").arg(++drags_);
        const QTransform toView = canvas_->sceneToViewTransform();
        if (!drawing_.isEmpty()) {
            if (drawing_.size() >= 2 && QLineF(toView.map(drawing_.first().p), e.view).length() <= kClose) {
                close();
                return;
            }
            drawing_.append(PathPoint::corner(e.scene));
            drag_ = Drag::NewAnchor;
            return;
        }
        if (const int i = editable(); i >= 0) {
            const MaskShape& s = project_->crop().shapes[i];
            index_ = i;
            startShape_ = pathedit::asPath(s);
            bool out = false;
            if (const int h = pathedit::hitHandle(s, toView, e.view, &out); h >= 0) {
                anchor_ = h;
                handleOut_ = out;
                drag_ = Drag::Handle;
                return;
            }
            if (const int a = shapeedit::hitAnchor(s, toView, e.view); a >= 0) {
                anchor_ = a;
                if (e.modifiers & Qt::AltModifier)
                    commit(pathedit::converted(s, a));
                else
                    drag_ = Drag::Anchor;
                return;
            }
            if (!(e.modifiers & Qt::ControlModifier))
                if (const auto hit = pathedit::hitSegment(s, toView, e.view); hit.segment >= 0) {
                    int added = -1;
                    commit(pathedit::inserted(s, hit.segment, hit.t, &added));
                    anchor_ = added;
                    return;
                }
        }
        anchor_ = -1;
        if (e.modifiers & Qt::ControlModifier) { // the direct selection
            const int hit = shapeedit::hitShape(project_->crop().shapes, e.scene);
            project_->setSelectedShapes(hit >= 0 ? QList<int>{hit} : QList<int>{});
            return;
        }
        project_->setSelectedShapes({});
        drawing_ = {PathPoint::corner(e.scene)};
        drag_ = Drag::NewAnchor;
    }

    void mouseMove(const ToolEvent& e) override {
        hover_ = e.scene;
        switch (drag_) {
        case Drag::NewAnchor: {
            PathPoint& pt = drawing_.last();
            if (QLineF(pt.p, e.scene).length() * canvas_->zoom() >= kDragPx) {
                pt.out = e.scene;
                pt.in = pt.p * 2 - e.scene;
                pt.smooth = true;
            }
            break;
        }
        case Drag::Handle: {
            MaskShape s = startShape_;
            pathedit::moveHandle(s.points[anchor_], handleOut_, e.scene, e.modifiers & Qt::AltModifier);
            commit(s, mergeKey_);
            break;
        }
        case Drag::Anchor: {
            MaskShape s = startShape_;
            PathPoint& pt = s.points[anchor_];
            const QPointF d = e.scene - start_;
            pt.p += d;
            pt.in += d;
            pt.out += d;
            commit(s, mergeKey_);
            break;
        }
        case Drag::None:
            break;
        }
    }

    void mouseRelease(const ToolEvent&) override { drag_ = Drag::None; }

    bool keyPress(QKeyEvent* e) override {
        if (e->key() == Qt::Key_Escape) {
            if (!drawing_.isEmpty())
                drawing_.clear();
            else if (anchor_ >= 0)
                anchor_ = -1;
            else
                project_->setSelectedShapes({});
            return true;
        }
        if ((e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) && !drawing_.isEmpty()) {
            close();
            return true;
        }
        if (e->key() != Qt::Key_Delete && e->key() != Qt::Key_Backspace) return false;
        const int i = editable();
        if (i < 0) return false;
        CropShape c = project_->crop();
        const MaskShape fewer = anchor_ >= 0 ? pathedit::removed(c.shapes[i], anchor_) : c.shapes[i];
        if (anchor_ >= 0 && fewer.points.size() < c.shapes[i].points.size()) {
            c.shapes[i] = fewer;
        } else {
            c.shapes.removeAt(i);
            project_->setSelectedShapes({});
        }
        anchor_ = -1;
        project_->changeCrop(c);
        return true;
    }

    bool wantsKey(const QKeyEvent* e) const override {
        switch (e->key()) {
        case Qt::Key_Escape:
            return !drawing_.isEmpty() || anchor_ >= 0 || !project_->selectedShapes().isEmpty();
        case Qt::Key_Return:
        case Qt::Key_Enter:
            return !drawing_.isEmpty();
        case Qt::Key_Delete:
        case Qt::Key_Backspace:
            return editable() >= 0;
        default:
            return false;
        }
    }

    Qt::CursorShape cursor(const ToolEvent& e) const override {
        hover_ = e.scene; // the next segment follows a buttonless move (the canvas asks only for the cursor)
        return Qt::CrossCursor;
    }

    void paintOverlay(QPainter& p) override {
        const ThemeTokens& t = Theme::instance().tokens();
        const QTransform toView = canvas_->sceneToViewTransform();
        p.setBrush(Qt::NoBrush);
        if (!drawing_.isEmpty()) {
            // The path so far, open, and the next segment to the pointer.
            QPainterPath line(drawing_.first().p);
            for (qsizetype i = 1; i < drawing_.size(); ++i)
                line.cubicTo(drawing_[i - 1].out, drawing_[i].in, drawing_[i].p);
            line.cubicTo(drawing_.last().out, hover_, hover_);
            p.setPen(QPen(t.accentHover, 1.25, Qt::DashLine));
            p.drawPath(toView.map(line));
            paintAnchors(p, toView, drawing_, int(drawing_.size()) - 1);
            return;
        }
        if (const int i = editable(); i >= 0) paintAnchors(p, toView, project_->crop().shapes[i].points, anchor_);
    }

    QWidget* createOptions(QWidget* parent) override { return createPenOptions(project_, &draft_, parent); }

private:
    enum class Drag { None, NewAnchor, Handle, Anchor };

    // The one selected shape the Pen edits — a path or a polygon, not locked — else −1.
    int editable() const {
        const QList<int> s = project_->selectedShapes();
        if (s.size() != 1) return -1;
        const MaskShape& m = project_->crop().shapes[s.first()];
        return !m.isStandard() && !m.locked ? s.first() : -1;
    }

    void commit(const MaskShape& s, const QString& mergeKey = {}) {
        CropShape c = project_->crop();
        if (index_ < 0 || index_ >= c.shapes.size()) return;
        c.shapes[index_] = s;
        project_->changeCrop(c, mergeKey);
    }

    void close() {
        const bool curved =
            std::any_of(drawing_.begin(), drawing_.end(), [](const PathPoint& pt) { return pt.hasHandles(); });
        CropShape c = project_->crop();
        if ((drawing_.size() >= 3 || (drawing_.size() == 2 && curved)) && c.shapes.size() < MaskShape::kMaxShapes) {
            MaskShape s;
            s.kind = MaskShape::Kind::Path;
            s.role = draft_.role;
            s.points = drawing_;
            s.name = maskshapes::nextName(c.shapes, s.kind);
            c.shapes.append(s);
            project_->changeCrop(c);
            project_->setSelectedShapes({int(c.shapes.size() - 1)});
        }
        drawing_.clear();
        drag_ = Drag::None;
    }

    void paintAnchors(QPainter& p, const QTransform& toView, const QList<PathPoint>& points, int selected) {
        const ThemeTokens& t = Theme::instance().tokens();
        for (qsizetype i = 0; i < points.size(); ++i) {
            const PathPoint& pt = points[i];
            const QPointF at = toView.map(pt.p);
            p.setPen(QPen(t.accent, 1));
            for (const QPointF& h : {pt.in, pt.out})
                if (h != pt.p) {
                    const QPointF hv = toView.map(h);
                    p.setBrush(Qt::NoBrush);
                    p.drawLine(at, hv);
                    p.setBrush(t.accent);
                    p.drawEllipse(hv, 3, 3);
                }
            p.setPen(QPen(t.accent, 1.25));
            p.setBrush(i == selected ? t.accent : QColor(Qt::white));
            p.drawRect(QRectF(at.x() - 3.5, at.y() - 3.5, 7, 7));
        }
    }

    ShapeDraft draft_;
    QList<PathPoint> drawing_;
    Drag drag_ = Drag::None;
    QPointF start_;
    mutable QPointF hover_;
    MaskShape startShape_;
    qsizetype index_ = -1;
    int anchor_ = -1;
    bool handleOut_ = true;
    QString mergeKey_;
    int drags_ = 0;
};

} // namespace

Tool* createPenTool(Canvas* canvas, Project* project) {
    return new PenTool(canvas, project);
}

} // namespace avloop
