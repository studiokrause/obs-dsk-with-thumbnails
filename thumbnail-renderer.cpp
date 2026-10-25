#include "thumbnail-renderer.hpp"

#include <algorithm>
#include <map>
#include <mutex>

#include <graphics/vec4.h>
#include <util/platform.h>

// Minimum time between two renders of the same thumbnail.
#define THUMBNAIL_INTERVAL_NS 100000000ULL
// Maximum number of thumbnails rendered per frame, to spread the GPU load.
#define THUMBNAILS_PER_FRAME 4

namespace {

struct Entry {
	obs_weak_source_t *weak = nullptr;
	std::map<void *, uint32_t> owners;
	gs_texrender_t *texrender = nullptr;
	gs_stagesurf_t *stagesurf = nullptr;
	uint32_t cx = 0;
	uint32_t cy = 0;
	bool staged = false;
	uint64_t stagedFrame = 0;
	uint64_t lastRender = 0;
	QImage image;
	uint64_t serial = 0;
};

std::mutex mutex;
std::map<obs_weak_source_t *, Entry> entries;
obs_weak_source_t *lastRendered = nullptr;
uint64_t frame = 0;
uint64_t lastSerial = 0;
bool running = false;

void destroy_entries(std::vector<Entry> &removed)
{
	if (removed.empty())
		return;
	obs_enter_graphics();
	for (auto &e : removed) {
		gs_texrender_destroy(e.texrender);
		gs_stagesurface_destroy(e.stagesurf);
	}
	obs_leave_graphics();
	for (auto &e : removed) {
		obs_source_t *source = obs_weak_source_get_source(e.weak);
		if (source) {
			obs_source_dec_showing(source);
			obs_source_release(source);
		}
		obs_weak_source_release(e.weak);
	}
	removed.clear();
}

void render_entry(Entry &e, uint64_t now)
{
	e.lastRender = now;
	obs_source_t *source = obs_weak_source_get_source(e.weak);
	if (!source)
		return;
	const uint32_t sw = obs_source_get_width(source);
	const uint32_t sh = obs_source_get_height(source);
	uint32_t width = 0;
	for (const auto &o : e.owners) {
		if (o.second > width)
			width = o.second;
	}
	if (!sw || !sh || !width) {
		obs_source_release(source);
		return;
	}
	const uint32_t cx = width;
	const uint32_t cy = std::max(1u, (uint32_t)((uint64_t)cx * sh / sw));

	if (!e.texrender)
		e.texrender = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
	if (!e.stagesurf || e.cx != cx || e.cy != cy) {
		gs_stagesurface_destroy(e.stagesurf);
		e.stagesurf = gs_stagesurface_create(cx, cy, GS_RGBA);
		e.cx = cx;
		e.cy = cy;
		e.staged = false;
	}

	gs_texrender_reset(e.texrender);
	if (gs_texrender_begin(e.texrender, cx, cy)) {
		struct vec4 clear;
		vec4_zero(&clear);
		gs_clear(GS_CLEAR_COLOR, &clear, 0.0f, 0);
		gs_ortho(0.0f, (float)sw, 0.0f, (float)sh, -100.0f, 100.0f);

		gs_blend_state_push();
		gs_blend_function_separate(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA, GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);
		obs_source_video_render(source);
		gs_blend_state_pop();

		gs_texrender_end(e.texrender);
		gs_stage_texture(e.stagesurf, gs_texrender_get_texture(e.texrender));
		e.staged = true;
		e.stagedFrame = frame;
	}
	obs_source_release(source);
}

void render_thumbnails(void *param, uint32_t cx, uint32_t cy)
{
	UNUSED_PARAMETER(param);
	UNUSED_PARAMETER(cx);
	UNUSED_PARAMETER(cy);

	std::lock_guard<std::mutex> lock(mutex);
	if (entries.empty())
		return;
	frame++;

	// Read back what was staged in a previous frame, so mapping does not stall the GPU.
	for (auto &it : entries) {
		Entry &e = it.second;
		if (!e.staged || e.stagedFrame >= frame)
			continue;
		uint8_t *data = nullptr;
		uint32_t linesize = 0;
		if (gs_stagesurface_map(e.stagesurf, &data, &linesize)) {
			e.image = QImage(data, (int)e.cx, (int)e.cy, (int)linesize, QImage::Format_RGBA8888_Premultiplied).copy();
			e.serial = ++lastSerial;
			gs_stagesurface_unmap(e.stagesurf);
		}
		e.staged = false;
	}

	// Render a few thumbnails per frame, round robin.
	const uint64_t now = os_gettime_ns();
	auto it = entries.upper_bound(lastRendered);
	int rendered = 0;
	for (size_t i = 0; i < entries.size() && rendered < THUMBNAILS_PER_FRAME; i++, it++) {
		if (it == entries.end())
			it = entries.begin();
		Entry &e = it->second;
		if (e.staged || now - e.lastRender < THUMBNAIL_INTERVAL_NS)
			continue;
		render_entry(e, now);
		lastRendered = it->first;
		rendered++;
	}
}

} // namespace

namespace ThumbnailRenderer {

void Init()
{
	std::lock_guard<std::mutex> lock(mutex);
	if (running)
		return;
	running = true;
	obs_add_main_render_callback(render_thumbnails, nullptr);
}

void Shutdown()
{
	std::vector<Entry> removed;
	{
		std::lock_guard<std::mutex> lock(mutex);
		if (!running)
			return;
		running = false;
	}
	obs_remove_main_render_callback(render_thumbnails, nullptr);
	{
		std::lock_guard<std::mutex> lock(mutex);
		for (auto &it : entries)
			removed.push_back(std::move(it.second));
		entries.clear();
		lastRendered = nullptr;
	}
	destroy_entries(removed);
}

void SetSources(void *owner, const std::vector<obs_source_t *> &sources, uint32_t width)
{
	std::vector<obs_source_t *> added;
	std::vector<Entry> removed;
	{
		std::lock_guard<std::mutex> lock(mutex);
		if (!running)
			return;
		std::vector<obs_weak_source_t *> wanted;
		for (auto source : sources) {
			obs_weak_source_t *weak = obs_source_get_weak_source(source);
			auto it = entries.find(weak);
			if (it == entries.end()) {
				Entry e;
				e.weak = weak;
				it = entries.emplace(weak, std::move(e)).first;
				added.push_back(obs_source_get_ref(source));
			} else {
				obs_weak_source_release(weak);
			}
			it->second.owners[owner] = width;
			wanted.push_back(it->first);
		}
		for (auto it = entries.begin(); it != entries.end();) {
			if (std::find(wanted.begin(), wanted.end(), it->first) == wanted.end()) {
				it->second.owners.erase(owner);
				if (it->second.owners.empty()) {
					if (lastRendered == it->first)
						lastRendered = nullptr;
					removed.push_back(std::move(it->second));
					it = entries.erase(it);
					continue;
				}
			}
			++it;
		}
	}
	// Sources rendered as thumbnails count as showing, like in the multiview.
	for (auto source : added) {
		if (source) {
			obs_source_inc_showing(source);
			obs_source_release(source);
		}
	}
	destroy_entries(removed);
}

void RemoveOwner(void *owner)
{
	SetSources(owner, {}, 0);
}

bool GetImage(obs_source_t *source, QImage &image, uint64_t &serial)
{
	obs_weak_source_t *weak = obs_source_get_weak_source(source);
	bool found = false;
	{
		std::lock_guard<std::mutex> lock(mutex);
		auto it = entries.find(weak);
		if (it != entries.end() && it->second.serial) {
			image = it->second.image;
			serial = it->second.serial;
			found = true;
		}
	}
	obs_weak_source_release(weak);
	return found;
}

} // namespace ThumbnailRenderer
