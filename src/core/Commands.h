#pragma once

#include "core/Project.h"
#include "core/Sequence.h"

#include <QElapsedTimer>
#include <QUndoCommand>

#include <functional>

namespace avloop {

// The Undo commands (spec §11.2): each holds a value before and after, never an image. Two commands of one kind and
// one merge key merge into one step when the key names a drag ("drag:…", one continuous gesture) or when they come
// within 500 ms of each other (repeated typing or wheel in one field).
template <typename T> class ValueCommand : public QUndoCommand {
public:
    using Apply = std::function<void(const T&)>;

    ValueCommand(int id, const QString& text, T before, T after, Apply apply, QString mergeKey)
        : QUndoCommand(text), id_(id), before_(std::move(before)), after_(std::move(after)), apply_(std::move(apply)),
          mergeKey_(std::move(mergeKey)) {
        clock_.start();
    }

    int id() const override { return mergeKey_.isEmpty() ? -1 : id_; }
    void redo() override { apply_(after_); }
    void undo() override { apply_(before_); }

    bool mergeWith(const QUndoCommand* other) override {
        const auto* o = dynamic_cast<const ValueCommand<T>*>(other);
        if (!o || o->id_ != id_ || o->mergeKey_ != mergeKey_) return false;
        const bool drag = mergeKey_.startsWith(QLatin1String("drag:"));
        if (!drag && clock_.elapsed() > kMergeWindowMs) return false;
        after_ = o->after_;
        clock_.restart();
        return true;
    }

    const T& before() const { return before_; }
    const T& after() const { return after_; }

    static constexpr qint64 kMergeWindowMs = 500;

private:
    int id_;
    T before_;
    T after_;
    Apply apply_;
    QString mergeKey_;
    QElapsedTimer clock_;
};

// The command ids (QUndoCommand::id) of spec §11.2's list.
enum CommandId {
    SetCropId = 1,
    SetTransformId,
    SetRangeId,
    SetLoopId,
    SetOutputId,
    SetQualityId,
    AddKeyframeId,
    MoveKeyframeId,
    DeleteKeyframeId,
    SetEffectsId,
    SetLayersId,
    SetSequenceId,
};

using SetLoopCommand = ValueCommand<LoopSettings>;
using SetOutputCommand = ValueCommand<OutputSettings>;
using SetQualityCommand = ValueCommand<QualitySettings>;
// The transform, the crop, In/Out, the keyframes, the effects, the layers and the sequence's own edits: each a step
// over the whole sequence (D-033, RS-4), its id the aspect's (so a drag in one field still merges).
using SetSequenceCommand = ValueCommand<Sequence>;

} // namespace avloop
