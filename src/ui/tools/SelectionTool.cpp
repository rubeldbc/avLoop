// The Selection (V, spec §6.2): a drag inside the video moves it; a drag on a corner handle scales it keeping its
// ratio about the opposite corner (Alt: about the centre). One drag is one Undo step. The video's border is a quiet
// blue-white dashed line with four handles; the crop frame stays still.
#include "core/Geometry.h"
#include "core/Project.h"
#include "ui/canvas/Canvas.h"
#include "ui/options/OptionsBar.h"
#include "ui/tools/Tool.h"

#include <QPainter>

#include <algorithm>
#include <cmath>
#include <optional>

namespace avloop {

namespace {

const QColor kBorder(0x7F, 0xB2, 0xFF); // the owner's mock: distinct from the crop's orange
constexpr double kHandle = 8;           // px
constexpr double kHit = 7;              // px around a handle's centre

using geometry::Corner;

class SelectionTool : public Tool {
public:
    SelectionTool(Canvas* canvas, Project* project) : Tool(QStringLiteral("select"), canvas, project) {}

    void mousePress(const ToolEvent& e) override {
        if (e.button != Qt::LeftButton || !project_->source().isValid()) return;
        start_ = e.scene;
        startT_ = project_->transform();
        mergeKey_ = QStringLiteral("drag:select:%1").arg(++drags_);
        // A video layer under the pointer (D-027) is picked first: the drag moves it.
        layer_ = videolayers::hitLayer(project_->layers(), e.scene, project_->time());
        project_->setSelectedLayer(layer_);
        if (layer_ >= 0) {
            layersAtStart_ = project_->layers();
            mode_ = Mode::Layer;
            return;
        }
        corner_ = cornerAt(e.view);
        mode_ = corner_                                                     ? Mode::Scale
                : startT_.rect(project_->source().size()).contains(e.scene) ? Mode::Move
                                                                            : Mode::None;
    }

    void mouseMove(const ToolEvent& e) override {
        if (mode_ == Mode::None) return;
        if (mode_ == Mode::Layer) {
            QList<VideoLayer> layers = layersAtStart_;
            layers[layer_].transform.x += e.scene.x() - start_.x();
            layers[layer_].transform.y += e.scene.y() - start_.y();
            project_->changeLayers(layers, mergeKey_);
            return;
        }
        VideoTransform t = startT_;
        if (mode_ == Mode::Move) {
            t.x += e.scene.x() - start_.x();
            t.y += e.scene.y() - start_.y();
        } else {
            t = scaled(e.scene, e.modifiers & Qt::AltModifier);
        }
        project_->changeTransform(t, mergeKey_);
    }

    void mouseRelease(const ToolEvent&) override { mode_ = Mode::None; }

    Qt::CursorShape cursor(const ToolEvent& e) const override {
        if (const auto c = cornerAt(e.view))
            return *c == Corner::TopLeft || *c == Corner::BottomRight ? Qt::SizeFDiagCursor : Qt::SizeBDiagCursor;
        return project_->transform().rect(project_->source().size()).contains(e.scene) ? Qt::SizeAllCursor
                                                                                       : Qt::ArrowCursor;
    }

    void paintOverlay(QPainter& p) override {
        if (!project_->source().isValid()) return;
        const QRectF r = canvas_->sceneToView(project_->transform().rect(project_->source().size()));
        QPen dashed(kBorder, 1, Qt::CustomDashLine);
        dashed.setDashPattern({5, 4});
        p.setPen(dashed);
        p.setBrush(Qt::NoBrush);
        p.drawRect(r.adjusted(0.5, 0.5, -0.5, -0.5));
        p.setPen(QPen(kBorder, 1.5));
        p.setBrush(Qt::white);
        for (Corner c : {Corner::TopLeft, Corner::TopRight, Corner::BottomRight, Corner::BottomLeft}) {
            const QPointF at = geometry::corner(r, c);
            p.drawRect(QRectF(at.x() - kHandle / 2, at.y() - kHandle / 2, kHandle, kHandle));
        }
    }

    QWidget* createOptions(QWidget* parent) override { return createSelectionOptions(project_, canvas_, parent); }

private:
    enum class Mode { None, Move, Scale, Layer };
    int layer_ = -1;
    QList<VideoLayer> layersAtStart_;

    std::optional<Corner> cornerAt(const QPointF& view) const {
        if (!project_->source().isValid()) return std::nullopt;
        const QRectF r = canvas_->sceneToView(project_->transform().rect(project_->source().size()));
        for (Corner c : {Corner::TopLeft, Corner::TopRight, Corner::BottomRight, Corner::BottomLeft}) {
            const QPointF d = view - geometry::corner(r, c);
            if (std::abs(d.x()) <= kHit && std::abs(d.y()) <= kHit) return c;
        }
        return std::nullopt;
    }

    // The corner follows the pointer; the scale is one number, so the larger of the two ratios wins.
    VideoTransform scaled(const QPointF& pointer, bool fromCentre) const {
        const QSizeF video = project_->source().size();
        const QRectF r0 = startT_.rect(video);
        VideoTransform t = startT_;
        if (fromCentre) {
            const QPointF c = r0.center();
            t.scale = std::max(std::abs(pointer.x() - c.x()) * 2 / video.width(),
                               std::abs(pointer.y() - c.y()) * 2 / video.height());
            return t.clamped();
        }
        const QPointF anchor = geometry::corner(r0, geometry::opposite(*corner_));
        t.scale = std::max(std::abs(pointer.x() - anchor.x()) / video.width(),
                           std::abs(pointer.y() - anchor.y()) / video.height());
        t = t.clamped();
        const QPointF grabbed = geometry::corner(r0, *corner_);
        const double sx = grabbed.x() >= anchor.x() ? 1 : -1;
        const double sy = grabbed.y() >= anchor.y() ? 1 : -1;
        t.x = anchor.x() + sx * video.width() * t.scale / 2;
        t.y = anchor.y() + sy * video.height() * t.scale / 2;
        return t;
    }

    Mode mode_ = Mode::None;
    QPointF start_;
    VideoTransform startT_;
    std::optional<Corner> corner_;
    QString mergeKey_;
    int drags_ = 0;
};

} // namespace

Tool* createSelectionTool(Canvas* canvas, Project* project) {
    return new SelectionTool(canvas, project);
}

} // namespace avloop
