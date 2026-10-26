// Undo (spec §11.2): every change is a command; one drag is one step whatever its length; typing or the wheel in one
// field merges within 500 ms; at most 200 steps; a new source clears the stack.
#include "core/Commands.h"
#include "core/Project.h"
#include "core/ProjectJson.h"

#include <QTest>
#include <QThread>

using namespace avloop;

namespace {

SourceInfo source() {
    SourceInfo s;
    s.path = QStringLiteral("x.mp4");
    s.width = 640;
    s.height = 360;
    s.fps = {30, 1};
    s.duration = 10;
    return s;
}

VideoTransform at(double x) {
    VideoTransform t = VideoTransform::initial({640, 360});
    t.x = x;
    return t;
}

} // namespace

class TestCommands : public QObject {
    Q_OBJECT
private slots:
    void undoRestores() {
        Project p;
        p.setSource(source());
        const VideoTransform before = p.transform();
        p.changeTransform(at(500));
        QCOMPARE(p.transform().x, 500.0);
        p.undoStack()->undo();
        QCOMPARE(p.transform(), before);
        p.undoStack()->redo();
        QCOMPARE(p.transform().x, 500.0);
    }
    void aDragIsOneStep() {
        Project p;
        p.setSource(source());
        for (int i = 0; i < 30; ++i) {
            p.changeTransform(at(400 + i), QStringLiteral("drag:select:1"));
            if (i == 10) QThread::msleep(700); // slow in the middle: still the same drag
        }
        QCOMPARE(p.undoStack()->count(), 1);
        p.changeTransform(at(10), QStringLiteral("drag:select:2"));
        QCOMPARE(p.undoStack()->count(), 2);
        p.undoStack()->undo();
        QCOMPARE(p.transform().x, 429.0);
    }
    void typingMergesWithin500ms() {
        Project p;
        p.setSource(source());
        p.changeTransform(at(1), QStringLiteral("field:x"));
        p.changeTransform(at(2), QStringLiteral("field:x"));
        QCOMPARE(p.undoStack()->count(), 1);
        p.changeTransform(at(3), QStringLiteral("field:y")); // another field
        QCOMPARE(p.undoStack()->count(), 2);
        QThread::msleep(650);
        p.changeTransform(at(4), QStringLiteral("field:y")); // after the window
        QCOMPARE(p.undoStack()->count(), 3);
    }
    void noMergeKeyNoMerge() {
        Project p;
        p.setSource(source());
        p.changeTransform(at(1));
        p.changeTransform(at(2));
        QCOMPARE(p.undoStack()->count(), 2);
    }
    void sameValueIsNoStep() {
        Project p;
        p.setSource(source());
        p.changeTransform(p.transform());
        QCOMPARE(p.undoStack()->count(), 0);
    }
    void limitIs200() {
        Project p;
        p.setSource(source());
        for (int i = 0; i < 250; ++i)
            p.changeTransform(at(i + 1));
        QCOMPARE(p.undoStack()->count(), Project::kUndoLimit);
    }
    void newSourceClears() {
        Project p;
        p.setSource(source());
        p.changeTransform(at(9));
        p.setSource(source());
        QCOMPARE(p.undoStack()->count(), 0);
        QCOMPARE(p.transform(), VideoTransform::initial({640, 360}));
    }
    void changedSaysWhat() {
        Project p;
        p.setSource(source());
        QList<Project::Aspect> seen;
        connect(&p, &Project::changed, this, [&](Project::Aspect a) { seen << a; });
        p.changeTransform(at(5));
        QCOMPARE(seen, QList<Project::Aspect>{Project::Aspect::Transform});
    }
    void rangeIsUndone() {
        // SetRange (spec §6.4): a new source's range is the whole file; a change is a step; a handle's drag one.
        Project p;
        p.setSource(source());
        QCOMPARE(p.range(), (TimeRange{0, 10}));
        QCOMPARE(p.outputFps(), (Rational{30, 1}));
        p.changeRange(timerange::withIn(p.range(), 2, 10, p.outputFps()));
        p.changeRange(timerange::withOut(p.range(), 4, 10, p.outputFps()));
        QCOMPARE(p.range(), (TimeRange{2, 4}));
        QCOMPARE(p.undoStack()->count(), 2);
        for (double t : {3.0, 3.5, 3.9})
            p.changeRange(timerange::dragOut(p.range(), t + 1, 10, p.outputFps()), QStringLiteral("drag:range:1"));
        QCOMPARE(p.undoStack()->count(), 3);
        QCOMPARE(p.range().out, 4.9);
        p.undoStack()->undo();
        QCOMPARE(p.range(), (TimeRange{2, 4}));
        p.undoStack()->undo();
        p.undoStack()->undo();
        QCOMPARE(p.range(), (TimeRange{0, 10}));
    }
    void everyCommandUndoes() {
        // Spec §11.2: SetCrop, SetTransform, SetRange, SetLoop, SetOutput, SetQuality — each undone and redone.
        Project p;
        p.setSource(source());
        const ProjectData before = p.snapshot();
        CropShape crop;
        crop.kind = CropShape::Kind::Ellipse;
        crop.rect = QRectF(10, 10, 200, 100);
        LoopSettings loop;
        loop.mode = LoopSettings::Mode::PingPong;
        OutputSettings output;
        output.setSizeId(QStringLiteral("long-512"));
        QualitySettings quality;
        quality.preset = QualitySettings::Preset::High;
        p.changeCrop(crop);
        p.changeTransform(at(400));
        p.changeRange(timerange::withIn(p.range(), 1, 10, p.outputFps()));
        p.changeLoop(loop);
        p.changeOutput(output);
        p.changeQuality(quality);
        QCOMPARE(p.undoStack()->count(), 6);
        const ProjectData after = p.snapshot();
        while (p.undoStack()->canUndo())
            p.undoStack()->undo();
        QCOMPARE(p.crop(), before.crop);
        QCOMPARE(p.transform(), before.transform);
        QCOMPARE(p.range(), before.range);
        QCOMPARE(p.loop(), before.loop);
        QCOMPARE(p.output(), before.output);
        QCOMPARE(p.quality(), before.quality);
        while (p.undoStack()->canRedo())
            p.undoStack()->redo();
        QCOMPARE(p.crop(), after.crop);
        QCOMPARE(p.output(), after.output);
        QCOMPARE(p.quality(), after.quality);
    }
    void aPresetIsOneStep() {
        // Spec §11.3: applying a preset changes Loop, Output and Quality in one Undo step; the same again is none.
        Project p;
        p.setSource(source());
        LoopSettings loop;
        loop.mode = LoopSettings::Mode::Simple;
        OutputSettings output;
        output.setSizeId(QStringLiteral("long-1280"));
        QualitySettings quality;
        quality.preset = QualitySettings::Preset::Low;
        p.changeSettings(loop, output, quality, QStringLiteral("Mine"));
        QCOMPARE(p.undoStack()->count(), 1);
        QCOMPARE(p.output().sizeId(), QStringLiteral("long-1280"));
        p.changeSettings(loop, output, quality, QStringLiteral("Mine"));
        QCOMPARE(p.undoStack()->count(), 1);
        p.undoStack()->undo();
        QCOMPARE(p.loop(), LoopSettings{});
        QCOMPARE(p.output().sizeId(), QStringLiteral("crop"));
        QCOMPARE(p.quality().preset, QualitySettings::Preset::Medium);
    }
};

QTEST_GUILESS_MAIN(TestCommands)
#include "test-commands.moc"
