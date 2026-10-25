// D3D11VA hardware decoding (spec §3.6): H.264, HEVC (8/10-bit), VP9, AV1, MPEG-2, VC-1 on the GPU; frames come down
// as NV12/P010 only when they are shown. Any failure falls back to software decoding, said once a file.
#include "media/DecoderImpl.h"

extern "C" {
#include <libavutil/hwcontext.h>
#include <libavutil/pixdesc.h>
}

#include <QDebug>

namespace avloop::media {

namespace {

AVPixelFormat pickD3D11(AVCodecContext*, const AVPixelFormat* formats) {
    for (const AVPixelFormat* p = formats; *p != AV_PIX_FMT_NONE; ++p)
        if (*p == AV_PIX_FMT_D3D11) return *p;
    // No hardware surface for this stream (a profile D3D11VA lacks): the first software format.
    for (const AVPixelFormat* p = formats; *p != AV_PIX_FMT_NONE; ++p) {
        const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(*p);
        if (desc && !(desc->flags & AV_PIX_FMT_FLAG_HWACCEL)) return *p;
    }
    return AV_PIX_FMT_NONE;
}

} // namespace

bool Decoder::Impl::setupHardware(AVCodecContext* c, const AVCodec* codec) {
    bool supported = false;
    for (int i = 0;; ++i) {
        const AVCodecHWConfig* cfg = avcodec_get_hw_config(codec, i);
        if (!cfg) break;
        if ((cfg->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX) && cfg->device_type == AV_HWDEVICE_TYPE_D3D11VA)
            supported = true;
    }
    if (!supported) {
        hwNote = QStringLiteral("HW decode off: no D3D11VA decoder for %1").arg(info.codec);
        return false;
    }
    if (!hwDevice) {
        AVBufferRef* device = nullptr;
        const int r = av_hwdevice_ctx_create(&device, AV_HWDEVICE_TYPE_D3D11VA, nullptr, nullptr, 0);
        if (r < 0) {
            hwNote = QStringLiteral("HW decode off: no D3D11 device (%1)").arg(avError(r));
            return false;
        }
        hwDevice.reset(device);
    }
    c->hw_device_ctx = av_buffer_ref(hwDevice.get());
    c->get_format = pickD3D11;
    c->extra_hw_frames = 4; // the decoder keeps prev, last and pending surfaces beside its own
    return true;
}

bool Decoder::Impl::toSoftware(const AVFrame* in, FramePtr& out) {
    out = makeFrame();
    if (!in->hw_frames_ctx) return av_frame_ref(out.get(), in) >= 0;
    const int r = av_hwframe_transfer_data(out.get(), in, 0);
    if (r < 0) {
        fallbackToSoftware(avError(r));
        return false;
    }
    av_frame_copy_props(out.get(), in);
    return true;
}

void Decoder::Impl::fallbackToSoftware(const QString& why) {
    if (!hwActive) return;
    hwNote = QStringLiteral("HW decode off: %1").arg(why);
    qWarning().noquote() << "[media]" << info.path << hwNote; // once a file: hardware is not tried again
    hwActive = false;
    QString error;
    openCodec(false, &error);
    needReseek = true;
}

} // namespace avloop::media
