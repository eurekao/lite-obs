#include "lite-obs/lite_obs_internal.h"

#include "lite-obs/lite_obs_core_video.h"
#include "lite-obs/lite_obs_core_audio.h"
#include "lite-obs/lite_obs_source.h"
#include "lite-obs/lite_encoder.h"
#include "lite-obs/output/aoa_output.h"
#include "lite-obs/output/file_output.h"
#include "lite-obs/output/srt_stream_output.h"
#include "lite-obs/output/rtmp_stream_output.h"
#include "lite-obs/output/iOS_muxd_output.h"
#include "lite-obs/util/threading.h"
#include "lite-obs/util/log.h"
#include "lite-obs/lite_obs_platform_config.h"

#include <set>


struct lite_obs_media_source_private
{
    std::shared_ptr<lite_obs_source> internal_source{};
    uintptr_t core_ptr{};
};

lite_obs_media_source_internal::lite_obs_media_source_internal()
{
    d_ptr = new lite_obs_media_source_private;
}

lite_obs_media_source_internal::~lite_obs_media_source_internal()
{
    delete d_ptr;
}

void lite_obs_media_source_internal::output_audio(const uint8_t *audio_data[MAX_AV_PLANES], uint32_t frames, audio_format format, speaker_layout layout, uint32_t sample_rate)
{
    lite_obs_source::lite_obs_source_audio_frame af{};

    for (int i=0; i<MAX_AV_PLANES; i++)
        af.data[i] = audio_data[i];

    af.frames = frames;
    af.format = format;
    af.samples_per_sec = sample_rate;
    af.speakers = layout;
    af.timestamp = os_gettime_ns();

    d_ptr->internal_source->lite_source_output_audio(af);
}

void lite_obs_media_source_internal::output_video(int texId, uint32_t width, uint32_t height)
{
    d_ptr->internal_source->lite_source_output_video(texId, width, height);
}

void lite_obs_media_source_internal::output_video(const uint8_t *video_data[MAX_AV_PLANES], const int line_size[MAX_AV_PLANES], video_format format, video_range_type range, video_colorspace color_space, uint32_t width, uint32_t height)
{
    d_ptr->internal_source->lite_source_output_video(video_data, line_size, format, range, color_space, width, height);
}

void lite_obs_media_source_internal::output_video(const uint8_t *img_data, uint32_t img_width, uint32_t img_height, bool is_bgra)
{
    d_ptr->internal_source->lite_source_output_video(img_data, img_width, img_height, is_bgra);
}

void lite_obs_media_source_internal::clear_video()
{
    d_ptr->internal_source->lite_source_clear_video();
}

void lite_obs_media_source_internal::set_pos(float x, float y)
{
    d_ptr->internal_source->lite_source_set_pos(x, y);
}

void lite_obs_media_source_internal::set_scale(float s_w, float s_h)
{
    d_ptr->internal_source->lite_source_set_scale(s_w, s_h);
}

void lite_obs_media_source_internal::set_rotate(float rot)
{
    d_ptr->internal_source->lite_source_set_rotate(rot);
}

void lite_obs_media_source_internal::set_render_box(int x, int y, int width, int height, source_aspect_ratio_mode mode)
{
    d_ptr->internal_source->lite_source_set_render_box(x, y, width, height, mode);
}

void lite_obs_media_source_internal::set_order(order_movement movement)
{
    std::lock_guard<std::recursive_mutex> lock(lite_obs_source::sources_mutex);
    auto &sources = lite_obs_source::sources[d_ptr->core_ptr];
    auto iter = std::find(sources.begin(), sources.end(), d_ptr->internal_source);
    if (iter == sources.end())
        return;

    switch (movement) {
    case MOVE_UP:
    {
        auto target = std::next(iter);
        if (target != sources.end())
            std::iter_swap(iter, target);
    }
    break;
    case MOVE_DOWN:
    {
        if (iter != sources.begin()) {
            auto target = std::prev(iter);
            std::iter_swap(iter, target);
        }
    }
    break;
    case MOVE_TOP:
    {
        auto target = std::prev(sources.end());
        if (iter != target)
            std::iter_swap(iter, target);
    }
    break;
    case MOVE_BOTTOM:
    {
        auto target = sources.begin();
        if (iter != target)
            std::iter_swap(iter, target);
    }
    break;
    default:
        break;
    }
}

void lite_obs_media_source_internal::set_flip(bool flip_h, bool flip_v)
{
    d_ptr->internal_source->lite_source_set_flip(flip_h, flip_v);
}

void lite_obs_media_source_internal::reset_transform()
{
    d_ptr->internal_source->lite_source_reset_transform();
}

struct lite_obs_private
{
    std::shared_ptr<lite_obs_core_video> video{};
    std::shared_ptr<lite_obs_core_audio> audio{};

    std::shared_ptr<lite_obs_output> output{};
    std::shared_ptr<lite_obs_encoder> video_encoder{};
    std::shared_ptr<lite_obs_encoder> audio_encoder{};

    std::set<lite_obs_media_source_internal *> sources;

    lite_obs_private() {
        auto ptr = reinterpret_cast<uintptr_t>(this);
        video = std::make_shared<lite_obs_core_video>(ptr);
        audio = std::make_shared<lite_obs_core_audio>(ptr);
    }

    ~lite_obs_private() {
        video_encoder.reset();
        audio_encoder.reset();

        video.reset();
        audio.reset();
    }
};

lite_obs_internal::lite_obs_internal()
{
    d_ptr = new lite_obs_private;
}

lite_obs_internal::~lite_obs_internal()
{
    {
        std::lock_guard<std::recursive_mutex> lock(lite_obs_source::sources_mutex);
        auto &sources = lite_obs_source::sources[reinterpret_cast<uintptr_t>(d_ptr)];
        sources.clear();
    }

    for (auto iter : d_ptr->sources) {
        delete iter;
    }
    d_ptr->sources.clear();

    if(d_ptr->output) {
        d_ptr->output->lite_obs_output_destroy();
        d_ptr->output.reset();
    }

    d_ptr->video->lite_obs_stop_video();
    d_ptr->audio->lite_obs_stop_audio();

    {
        std::lock_guard<std::recursive_mutex> lock(lite_obs_source::sources_mutex);
        lite_obs_source::sources.erase(reinterpret_cast<uintptr_t>(d_ptr));
    }

    delete d_ptr;
}

#define OBS_SIZE_MIN 2
#define OBS_SIZE_MAX (32 * 1024)

static inline bool size_valid(uint32_t width, uint32_t height)
{
    return (width >= OBS_SIZE_MIN && height >= OBS_SIZE_MIN && width <= OBS_SIZE_MAX && height <= OBS_SIZE_MAX);
}

int lite_obs_internal::obs_reset_video(uint32_t width, uint32_t height, uint32_t fps)
{
    if (d_ptr->video->lite_obs_video_active())
        return LITE_OBS_VIDEO_CURRENTLY_ACTIVE;

    if (!size_valid(width, height))
        return LITE_OBS_VIDEO_INVALID_PARAM;

    d_ptr->video->lite_obs_stop_video();

    width &= 0xFFFFFFFC;
    height &= 0xFFFFFFFE;

    return d_ptr->video->lite_obs_start_video(width, height, fps);
}

bool lite_obs_internal::obs_reset_audio(uint32_t sample_rate)
{
    return d_ptr->audio->lite_obs_start_audio(sample_rate);
}

bool lite_obs_internal::lite_obs_start_output(output_type type, void *output_info, int vb, int ab, const lite_obs_output_callbak &callback)
{
    if (!d_ptr->video->lite_obs_video_ready()) {
        blog(LOG_ERROR, "video inactive, start fail");
        return false;
    }

    if (!d_ptr->output)
    {
        std::shared_ptr<lite_obs_output> output = nullptr;
        switch (type) {
        case output_type::rtmp:
            output = std::make_shared<rtmp_stream_output>();
            break;
        case output_type::srt:
            output = std::make_shared<mpeg_ts_output>();
            break;
        case output_type::file:
            output = std::make_shared<lite_ffmpeg_mux>();
            break;
        case output_type::android_aoa:
            output = std::make_shared<aoa_output>();
            break;
        case output_type::iOS_usb:
            output = std::make_shared<iOS_muxd_output>();
            break;
        default:
            break;
        }

        if (!output)
            return false;

        output->i_set_output_info(output_info);
        if (!output->lite_obs_output_create(d_ptr->video, d_ptr->audio))
            return false;

        d_ptr->output = output;
        d_ptr->output->set_output_signal_callback(callback);

#if TARGET_PLATFORM == PLATFORM_ANDROID
        d_ptr->video_encoder = std::make_shared<lite_obs_encoder>(lite_obs_encoder::encoder_id::MEDIACODEC, vb, 0);
#elif TARGET_PLATFORM == PLATFORM_MAC || TARGET_PLATFORM == PLATFORM_IOS
        d_ptr->video_encoder = std::make_shared<lite_obs_encoder>(lite_obs_encoder::encoder_id::VIDEOTOOLBOX, vb, 0);
#else
        d_ptr->video_encoder = std::make_shared<lite_obs_encoder>(lite_obs_encoder::encoder_id::FFMPEG_H264_HW, vb, 0);
#endif
        d_ptr->video_encoder->lite_obs_encoder_set_core_video(d_ptr->video);
        d_ptr->audio_encoder = std::make_shared<lite_obs_encoder>(lite_obs_encoder::encoder_id::AAC, ab, 0);
        d_ptr->audio_encoder->lite_obs_encoder_set_core_audio(d_ptr->audio);
    }

    d_ptr->output->lite_obs_output_set_video_encoder(d_ptr->video_encoder);
    d_ptr->output->lite_obs_output_set_audio_encoder(d_ptr->audio_encoder, 0);
    return d_ptr->output->lite_obs_output_start();
}

void lite_obs_internal::lite_obs_stop_output()
{
    if(!d_ptr->output)
        return;

    d_ptr->output->lite_obs_output_stop();
}

lite_obs_media_source_internal *lite_obs_internal::lite_obs_create_source(source_type type)
{
    auto source = std::make_shared<lite_obs_source>(type, d_ptr->video, d_ptr->audio);
    auto source_wrapper = new lite_obs_media_source_internal;
    source_wrapper->d_ptr->internal_source = source;
    source_wrapper->d_ptr->core_ptr = reinterpret_cast<uintptr_t>(d_ptr);
    d_ptr->sources.emplace(source_wrapper);

    {
        std::lock_guard<std::recursive_mutex> lock(lite_obs_source::sources_mutex);
        auto &sources = lite_obs_source::sources[reinterpret_cast<uintptr_t>(d_ptr)];
        sources.push_back(source);
    }

    return source_wrapper;
}

void lite_obs_internal::lite_obs_destroy_source(lite_obs_media_source_internal *source)
{
    {
        std::lock_guard<std::recursive_mutex> lock(lite_obs_source::sources_mutex);
        auto &sources = lite_obs_source::sources[reinterpret_cast<uintptr_t>(d_ptr)];
        sources.erase(std::find(sources.begin(), sources.end(), source->d_ptr->internal_source));
    }

    d_ptr->sources.erase(source);
    delete source;
}


void lite_obs_internal::lite_obs_reset_encoder(bool sw)
{
    if (d_ptr->video_encoder) {
        if (sw)
            d_ptr->video_encoder->lite_obs_encoder_reset_encoder_impl(lite_obs_encoder::encoder_id::X264);
        else {
#if TARGET_PLATFORM == PLATFORM_ANDROID
            d_ptr->video_encoder->lite_obs_encoder_reset_encoder_impl(lite_obs_encoder::encoder_id::MEDIACODEC);
#elif TARGET_PLATFORM == PLATFORM_MAC || TARGET_PLATFORM == PLATFORM_IOS
            d_ptr->video_encoder->lite_obs_encoder_reset_encoder_impl(lite_obs_encoder::encoder_id::VIDEOTOOLBOX);
#else
            d_ptr->video_encoder->lite_obs_encoder_reset_encoder_impl(lite_obs_encoder::encoder_id::FFMPEG_H264_HW);
#endif
        }
    }
}


