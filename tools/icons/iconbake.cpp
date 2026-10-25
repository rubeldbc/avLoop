// avloop-iconbake: a developer's tool, never shipped. Renders an SVG at several sizes and writes a Windows .ico whose
// entries are PNG images (spec §5.3: avloop.ico with 16, 24, 32, 48, 64, 128 and 256 px from one SVG).
//   avloop-iconbake <in.svg> <out.ico> [size …]
#include <QBuffer>
#include <QByteArray>
#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QSvgRenderer>
#include <QtEndian>

#include <cstdio>
#include <vector>

namespace {

void put16(QByteArray& b, quint16 v) {
    const quint16 le = qToLittleEndian(v);
    b.append(reinterpret_cast<const char*>(&le), 2);
}

void put32(QByteArray& b, quint32 v) {
    const quint32 le = qToLittleEndian(v);
    b.append(reinterpret_cast<const char*>(&le), 4);
}

} // namespace

int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);
    const QStringList args = QGuiApplication::arguments();
    if (args.size() < 3) {
        std::fputs("usage: avloop-iconbake <in.svg> <out.ico> [size ...]\n", stderr);
        return 1;
    }
    QSvgRenderer renderer(args.at(1));
    if (!renderer.isValid()) {
        std::fputs("cannot read the SVG\n", stderr);
        return 1;
    }
    std::vector<int> sizes;
    for (qsizetype i = 3; i < args.size(); ++i)
        sizes.push_back(args.at(i).toInt());
    if (sizes.empty()) sizes = {16, 24, 32, 48, 64, 128, 256};

    std::vector<QByteArray> pngs;
    for (int s : sizes) {
        QImage image(s, s, QImage::Format_ARGB32);
        image.fill(Qt::transparent);
        QPainter p(&image);
        p.setRenderHint(QPainter::Antialiasing);
        renderer.render(&p);
        p.end();
        QByteArray png;
        QBuffer buffer(&png);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "PNG");
        pngs.push_back(png);
    }

    QByteArray ico;
    put16(ico, 0);
    put16(ico, 1);
    put16(ico, static_cast<quint16>(sizes.size()));
    quint32 offset = 6 + 16 * static_cast<quint32>(sizes.size());
    for (size_t i = 0; i < sizes.size(); ++i) {
        const int s = sizes[i];
        ico.append(static_cast<char>(s >= 256 ? 0 : s));
        ico.append(static_cast<char>(s >= 256 ? 0 : s));
        ico.append('\0');
        ico.append('\0');
        put16(ico, 1);
        put16(ico, 32);
        put32(ico, static_cast<quint32>(pngs[i].size()));
        put32(ico, offset);
        offset += static_cast<quint32>(pngs[i].size());
    }
    for (const QByteArray& png : pngs)
        ico.append(png);

    QFile out(args.at(2));
    if (!out.open(QIODevice::WriteOnly) || out.write(ico) != ico.size()) {
        std::fputs("cannot write the .ico\n", stderr);
        return 1;
    }
    std::printf("wrote %s: %zu sizes, %lld bytes\n", qPrintable(args.at(2)), sizes.size(),
                static_cast<long long>(ico.size()));
    return 0;
}
