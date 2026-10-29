// The side panel's Clip effects section (D-017): On (off keeps the values, shows the clip as it is) and Reset; Light —
// Exposure, Contrast, Highlights, Shadows, Whites, Blacks; Colour — Temperature, Tint, Saturation, Vibrance, Hue;
// the colour theme (D-018) — Replace colour (From, picked from the canvas by the eyedropper, To, Hue range, Softness,
// Amount), Colourise, Split toning, Duotone; Creative — Sepia, Invert, Posterize, Vignette and its size, Opacity;
// Detail — Sharpen, Blur. Every edit is one Undo step (typing or the wheel in one field merges); a theme's colour
// chosen while its Amount is 0 sets it to 100 %. The canvas shows the effects at once, the Preview and the export
// exactly.
#include "core/Project.h"
#include "ui/WalkRunner.h"
#include "ui/panels/Section.h"
#include "ui/panels/SidePanel.h"
#include "ui/widgets/ColourField.h"
#include "ui/widgets/ResetLabel.h"
#include "ui/widgets/StepSpinBox.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

namespace avloop {

namespace {

QLabel* label(const QString& text, const char* property, QWidget* parent) {
    auto* l = new QLabel(text, parent);
    l->setProperty(property, true);
    l->setWordWrap(true);
    return l;
}

// The values that differ from none, "key=value" by the file's keys (what walks read).
QString summary(const ClipEffects& e) {
    const QJsonObject now = e.toJson();
    const QJsonObject none = ClipEffects{}.toJson();
    QStringList changed;
    for (auto it = now.begin(); it != now.end(); ++it)
        if (it.value() != none.value(it.key()))
            changed << QStringLiteral("%1=%2").arg(it.key(), it.value().isBool() ? QString::number(it.value().toBool())
                                                             : it.value().isString()
                                                                 ? it.value().toString()
                                                                 : QString::number(it.value().toDouble()));
    return changed.join(QLatin1Char(' '));
}

} // namespace

void SidePanel::bindEffects(Project* project, const ColourField::Picker& pick) {
    Section* s = effects_;
    QWidget* body = s->body();
    QLabel* hint = label(tr("Open a video to set its effects."), "quiet", body);
    s->addWide(hint);

    auto* top = new QWidget(body);
    auto* topRow = new QHBoxLayout(top);
    topRow->setContentsMargins(0, 0, 0, 0);
    auto* on = new QCheckBox(tr("On"), top);
    on->setObjectName(QStringLiteral("panel.effects.on"));
    on->setToolTip(tr("Off shows the clip as it is; the values stay"));
    auto* reset = new QPushButton(tr("Reset"), top);
    reset->setObjectName(QStringLiteral("panel.effects.reset"));
    reset->setToolTip(tr("Every effect back to none"));
    topRow->addWidget(on);
    topRow->addStretch(1);
    topRow->addWidget(reset);
    s->addWide(top);

    // The numbers: each one field of ClipEffects.
    struct Number {
        StepSpinBox* box;
        double ClipEffects::* value;
    };
    QList<Number> numbers;
    QList<QWidget*> fields{on, reset};
    auto number = [&](const char* key, const QString& name, double min, double max, double step, int decimals,
                      const QString& suffix, double ClipEffects::* value) {
        auto* box =
            new StepSpinBox(QStringLiteral("panel.effects.%1").arg(QLatin1String(key)), min, max, step, decimals, body);
        box->setSuffix(suffix);
        s->addRow(name, box);
        reset::setDefault(box, ClipEffects{}.*value); // its label resets it (D-025)
        numbers.append({box, value});
        fields << box;
        connect(box, &StepSpinBox::valueChanged, this, [project, key, value](double v) {
            ClipEffects e = project->effects();
            e.*value = v;
            project->changeEffects(e, QStringLiteral("field:effects.%1").arg(QLatin1String(key)));
        });
        return box;
    };
    const QString percent = tr(" %");
    s->addWide(label(tr("Light"), "subhead", body));
    number("exposure", tr("Exposure"), -4, 4, 0.1, 1, QString(), &ClipEffects::exposure)
        ->setToolTip(tr("Stops of light (EV), -4 to 4"));
    number("contrast", tr("Contrast"), -100, 100, 1, 0, QString(), &ClipEffects::contrast);
    number("highlights", tr("Highlights"), -100, 100, 1, 0, QString(), &ClipEffects::highlights);
    number("shadows", tr("Shadows"), -100, 100, 1, 0, QString(), &ClipEffects::shadows);
    number("whites", tr("Whites"), -100, 100, 1, 0, QString(), &ClipEffects::whites);
    number("blacks", tr("Blacks"), -100, 100, 1, 0, QString(), &ClipEffects::blacks);
    s->addWide(label(tr("Colour"), "subhead", body));
    number("temperature", tr("Temperature"), -100, 100, 1, 0, QString(), &ClipEffects::temperature)
        ->setToolTip(tr("Below 0 cooler, above 0 warmer"));
    number("tint", tr("Tint"), -100, 100, 1, 0, QString(), &ClipEffects::tint)
        ->setToolTip(tr("Below 0 greener, above 0 more magenta"));
    number("saturation", tr("Saturation"), 0, 200, 1, 0, percent, &ClipEffects::saturation);
    number("vibrance", tr("Vibrance"), -100, 100, 1, 0, QString(), &ClipEffects::vibrance)
        ->setToolTip(tr("Saturation that spares colours already strong"));
    number("hue", tr("Hue"), -180, 180, 1, 0, tr("°"), &ClipEffects::hue);

    // The colour theme: each effect's colours, then its Amount; a colour chosen at 0 % turns it full on.
    struct Colour {
        ColourField* field;
        std::uint32_t ClipEffects::* value;
    };
    QList<Colour> colours;
    auto colour = [&](const char* key, const QString& name, std::uint32_t ClipEffects::* value,
                      double ClipEffects::* amount) {
        auto* field = new ColourField(QStringLiteral("panel.effects.%1").arg(QLatin1String(key)), name, body);
        s->addRow(name, field);
        reset::setDefault(field, QColor::fromRgb(QRgb(ClipEffects{}.*value)));
        colours.append({field, value});
        fields << field;
        connect(field, &ColourField::colourChosen, this, [project, value, amount](const QColor& c) {
            ClipEffects e = project->effects();
            e.*value = c.rgb() & 0xffffffu;
            if (e.*amount == 0) e.*amount = 100;
            project->changeEffects(e);
        });
        return field;
    };
    s->addWide(label(tr("Replace colour"), "subhead", body));
    colour("replaceFrom", tr("From"), &ClipEffects::replaceFrom, &ClipEffects::replace)->setPicker(pick);
    colour("replaceTo", tr("To"), &ClipEffects::replaceTo, &ClipEffects::replace);
    number("replaceRange", tr("Hue range"), 0, 180, 1, 0, tr("°"), &ClipEffects::replaceRange)
        ->setToolTip(tr("Hues this near the From colour change fully"));
    number("replaceSoftness", tr("Softness"), 0, 180, 1, 0, tr("°"), &ClipEffects::replaceSoftness)
        ->setToolTip(tr("Hues beyond the range change less and less"));
    number("replace", tr("Amount"), 0, 100, 1, 0, percent, &ClipEffects::replace);
    s->addWide(label(tr("Colourise"), "subhead", body));
    colour("colouriseColour", tr("Colour"), &ClipEffects::colouriseColour, &ClipEffects::colourise)
        ->setToolTip(tr("Every pixel this colour, its lightness kept"));
    number("colourise", tr("Amount"), 0, 100, 1, 0, percent, &ClipEffects::colourise);
    s->addWide(label(tr("Split toning"), "subhead", body));
    colour("splitShadows", tr("Shadows"), &ClipEffects::splitShadows, &ClipEffects::split);
    colour("splitHighlights", tr("Highlights"), &ClipEffects::splitHighlights, &ClipEffects::split);
    number("split", tr("Amount"), 0, 100, 1, 0, percent, &ClipEffects::split);
    s->addWide(label(tr("Duotone"), "subhead", body));
    colour("duotoneDark", tr("Dark"), &ClipEffects::duotoneDark, &ClipEffects::duotone)
        ->setToolTip(tr("What black becomes"));
    colour("duotoneLight", tr("Light"), &ClipEffects::duotoneLight, &ClipEffects::duotone)
        ->setToolTip(tr("What white becomes"));
    number("duotone", tr("Amount"), 0, 100, 1, 0, percent, &ClipEffects::duotone);
    s->addWide(label(tr("Creative"), "subhead", body));
    number("sepia", tr("Sepia"), 0, 100, 1, 0, percent, &ClipEffects::sepia);
    auto* invert = new QCheckBox(tr("Invert"), body);
    invert->setObjectName(QStringLiteral("panel.effects.invert"));
    invert->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed); // a click beside its text is not on it
    s->addRow(QString(), invert);
    fields << invert;
    auto* posterize = new StepSpinBox(QStringLiteral("panel.effects.posterize"), 0, 32, 1, 0, body);
    posterize->setSpecialValueText(tr("Off"));
    posterize->setToolTip(tr("Levels a colour channel keeps, 2 to 32"));
    s->addRow(tr("Posterize"), posterize);
    fields << posterize;
    number("vignette", tr("Vignette"), -100, 100, 1, 0, QString(), &ClipEffects::vignette)
        ->setToolTip(tr("Above 0 darker edges, below 0 lighter"));
    number("vignetteSize", tr("Vignette size"), 0, 100, 1, 0, QString(), &ClipEffects::vignetteSize)
        ->setToolTip(tr("How far the clear middle reaches"));
    number("opacity", tr("Opacity"), 0, 100, 1, 0, percent, &ClipEffects::opacity);
    s->addWide(label(tr("Detail"), "subhead", body));
    number("sharpen", tr("Sharpen"), 0, 100, 1, 0, QString(), &ClipEffects::sharpen);
    number("blur", tr("Blur"), 0, 100, 1, 0, tr(" px"), &ClipEffects::blur)
        ->setToolTip(tr("Radius in the video's own pixels"));

    auto refresh = [=] {
        const ClipEffects& e = project->effects();
        const bool open = project->source().isValid();
        hint->setVisible(!open);
        for (QWidget* w : fields)
            w->setEnabled(open);
        const QSignalBlocker b1(on), b2(invert);
        on->setChecked(e.on);
        invert->setChecked(e.invert);
        posterize->setValueQuietly(e.posterize);
        for (const Number& n : numbers)
            n.box->setValueQuietly(e.*(n.value));
        for (const Colour& c : colours)
            c.field->setColourQuietly(QColor::fromRgb(QRgb(e.*(c.value))));
    };
    connect(project, &Project::changed, this, [refresh](Project::Aspect a) {
        if (a == Project::Aspect::Effects || a == Project::Aspect::Source) refresh();
    });
    connect(on, &QCheckBox::clicked, this, [project](bool checked) {
        ClipEffects e = project->effects();
        e.on = checked;
        project->changeEffects(e);
    });
    connect(invert, &QCheckBox::clicked, this, [project](bool checked) {
        ClipEffects e = project->effects();
        e.invert = checked;
        project->changeEffects(e);
    });
    connect(posterize, &StepSpinBox::valueChanged, this, [project, posterize](double v) {
        // One level is no picture: a step up from Off goes to 2, a step down from 2 to Off.
        ClipEffects e = project->effects();
        e.posterize = int(v) == 1 ? (e.posterize >= 2 ? 0 : 2) : int(v);
        posterize->setValueQuietly(e.posterize);
        project->changeEffects(e, QStringLiteral("field:effects.posterize"));
    });
    connect(reset, &QPushButton::clicked, this, [project] { project->changeEffects(ClipEffects{}); });

    WalkProbes::add(QStringLiteral("effects.summary"), [project] { return summary(project->effects()); });
    WalkProbes::add(QStringLiteral("effects.on"), [project] { return project->effects().on; });
    reset::setDefault(posterize, 0.0);
    reset::setDefault(invert, false);
    reset::setDefault(on, true);
    s->addResetAction(tr("Reset Clip effects"), QStringLiteral("effects"),
                      [project] { project->changeEffects(ClipEffects{}); });
    refresh();
}

} // namespace avloop
